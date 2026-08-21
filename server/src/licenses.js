const crypto = require('crypto');
const { pool } = require('./db');
const { hashPassword } = require('./auth');
const { findPlan } = require('./plans');

// Whether a user's VIP grant is currently usable.
function isActive(user) {
    if (!user || user.blocked || !user.is_vip) return false;
    if (user.expires_at && new Date(user.expires_at).getTime() <= Date.now()) return false;
    return true;
}

async function getUserByUsername(username) {
    const { rows } = await pool.query('SELECT * FROM users WHERE username = $1', [username]);
    return rows[0] || null;
}

async function getUserById(id) {
    const { rows } = await pool.query('SELECT * FROM users WHERE id = $1', [id]);
    return rows[0] || null;
}

async function getUserByTelegramId(telegramId) {
    const { rows } = await pool.query('SELECT * FROM users WHERE telegram_id = $1', [telegramId]);
    return rows[0] || null;
}

function randomPassword(len = 10) {
    return crypto.randomBytes(len).toString('base64url').slice(0, len);
}

// Creates a brand-new account with no VIP grant yet (grantVip is a separate
// step, called once payment/admin approval lands).
async function createUser({ username, password, telegramId = null }) {
    const passwordHash = await hashPassword(password);
    const { rows } = await pool.query(
        `INSERT INTO users (username, password_hash, telegram_id)
         VALUES ($1, $2, $3) RETURNING *`,
        [username, passwordHash, telegramId]
    );
    return rows[0];
}

async function grantVip(username, days) {
    const user = await getUserByUsername(username);
    if (!user) return null;

    // Extend from current expiry if still active, otherwise from now.
    const base = (user.expires_at && new Date(user.expires_at).getTime() > Date.now())
        ? new Date(user.expires_at)
        : new Date();
    const expiresAt = new Date(base.getTime() + days * 24 * 60 * 60 * 1000);

    const { rows } = await pool.query(
        `UPDATE users SET is_vip = TRUE, expires_at = $2, blocked = FALSE
         WHERE username = $1 RETURNING *`,
        [username, expiresAt]
    );
    return rows[0];
}

async function revokeVip(username) {
    const { rows } = await pool.query(
        `UPDATE users SET is_vip = FALSE WHERE username = $1 RETURNING *`,
        [username]
    );
    return rows[0] || null;
}

async function blockUser(username, blocked) {
    const { rows } = await pool.query(
        `UPDATE users SET blocked = $2 WHERE username = $1 RETURNING *`,
        [username, blocked]
    );
    return rows[0] || null;
}

async function touchHeartbeat(userId) {
    await pool.query(
        `UPDATE users SET last_online_at = now(), total_minutes = total_minutes + 1 WHERE id = $1`,
        [userId]
    );
}

async function listUsers(limit = 50) {
    const { rows } = await pool.query(
        `SELECT username, is_vip, expires_at, blocked, created_at, last_online_at
         FROM users ORDER BY created_at DESC LIMIT $1`,
        [limit]
    );
    return rows;
}

async function getStats() {
    const { rows } = await pool.query(`
        SELECT
            COUNT(*)::int AS total_users,
            COUNT(*) FILTER (WHERE is_vip AND (expires_at IS NULL OR expires_at > now()) AND NOT blocked)::int AS active_vip,
            COUNT(*) FILTER (WHERE blocked)::int AS blocked_users
        FROM users
    `);
    return rows[0];
}

// -----------------------------------------------------------------------
//  Balance
// -----------------------------------------------------------------------
async function addBalance(username, amount) {
    const { rows } = await pool.query(
        `UPDATE users SET balance_som = balance_som + $2 WHERE username = $1 RETURNING *`,
        [username, amount]
    );
    return rows[0] || null;
}

async function createTopupRequest(userId, telegramId, amount) {
    const { rows } = await pool.query(
        `INSERT INTO balance_topups (user_id, telegram_id, amount_som) VALUES ($1, $2, $3) RETURNING *`,
        [userId, telegramId, amount]
    );
    return rows[0];
}

// Called after an admin manually credits balance, so the oldest pending
// request for that user stops showing up as outstanding.
async function confirmOldestPendingTopup(userId) {
    await pool.query(
        `UPDATE balance_topups SET status = 'confirmed', confirmed_at = now()
         WHERE id = (
             SELECT id FROM balance_topups
             WHERE user_id = $1 AND status = 'pending'
             ORDER BY created_at ASC LIMIT 1
         )`,
        [userId]
    );
}

// Atomically deducts the plan price from the user's balance and extends
// their VIP. Returns { ok: true, user } or { ok: false, error }.
async function purchaseVip(userId, planKey) {
    const plan = findPlan(planKey);
    if (!plan) return { ok: false, error: 'Noma\'lum reja' };

    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        const { rows } = await client.query('SELECT * FROM users WHERE id = $1 FOR UPDATE', [userId]);
        const user = rows[0];
        if (!user) { await client.query('ROLLBACK'); return { ok: false, error: 'Hisob topilmadi' }; }
        if (user.blocked) { await client.query('ROLLBACK'); return { ok: false, error: 'Hisob bloklangan' }; }
        if (Number(user.balance_som) < plan.price) {
            await client.query('ROLLBACK');
            return { ok: false, error: 'insufficient_balance', needed: plan.price - Number(user.balance_som) };
        }

        const base = (user.expires_at && new Date(user.expires_at).getTime() > Date.now())
            ? new Date(user.expires_at)
            : new Date();
        const expiresAt = new Date(base.getTime() + plan.days * 24 * 60 * 60 * 1000);

        const upd = await client.query(
            `UPDATE users SET balance_som = balance_som - $2, is_vip = TRUE, expires_at = $3
             WHERE id = $1 RETURNING *`,
            [userId, plan.price, expiresAt]
        );

        await client.query(
            `INSERT INTO payments (user_id, telegram_id, provider, amount_tiyin, plan_days, status, paid_at)
             VALUES ($1, $2, 'balance', $3, $4, 'paid', now())`,
            [userId, user.telegram_id, plan.price * 100, plan.days]
        );

        await client.query('COMMIT');
        return { ok: true, user: upd.rows[0], plan };
    } catch (err) {
        await client.query('ROLLBACK');
        throw err;
    } finally {
        client.release();
    }
}

async function listPendingTopups(limit = 20) {
    const { rows } = await pool.query(
        `SELECT bt.*, u.username FROM balance_topups bt
         JOIN users u ON u.id = bt.user_id
         WHERE bt.status = 'pending'
         ORDER BY bt.created_at ASC LIMIT $1`,
        [limit]
    );
    return rows;
}

// Confirms one specific top-up request by id (used by the admin panel's
// "Kutilayotgan to'lovlar" list) — credits exactly that request's amount.
async function confirmTopupById(topupId) {
    const client = await pool.connect();
    try {
        await client.query('BEGIN');

        const { rows } = await client.query(
            `SELECT * FROM balance_topups WHERE id = $1 AND status = 'pending' FOR UPDATE`,
            [topupId]
        );
        const topup = rows[0];
        if (!topup) { await client.query('ROLLBACK'); return null; }

        const upd = await client.query(
            `UPDATE users SET balance_som = balance_som + $2 WHERE id = $1 RETURNING *`,
            [topup.user_id, topup.amount_som]
        );
        await client.query(
            `UPDATE balance_topups SET status = 'confirmed', confirmed_at = now() WHERE id = $1`,
            [topupId]
        );

        await client.query('COMMIT');
        return { user: upd.rows[0], topup };
    } catch (err) {
        await client.query('ROLLBACK');
        throw err;
    } finally {
        client.release();
    }
}

// ─── Paylov (avtomatik to'ldirish) ────────────────────────────────

// Paylov checkout yaratilgach chaqiriladi: to'ldirish so'rovini 'pending'
// holatida yozadi va Paylov buyurtmasiga bog'laydi. Tasdiqlash uchun
// confirmTopupById() ishlatiladi — u atomik va ikki marta ishlamaydi.
async function createPaylovTopup(userId, telegramId, amountSom, orderId, externalId, provider) {
    const { rows } = await pool.query(
        `INSERT INTO balance_topups (user_id, telegram_id, amount_som, paylov_order_id, external_id, provider)
         VALUES ($1, $2, $3, $4, $5, $6) RETURNING *`,
        [userId, telegramId, amountSom, orderId, externalId, provider]
    );
    return rows[0];
}

async function getTopupByOrderId(orderId) {
    const { rows } = await pool.query(
        `SELECT * FROM balance_topups WHERE paylov_order_id = $1 LIMIT 1`,
        [orderId]
    );
    return rows[0] || null;
}

// Bot qayta ishga tushganda tugallanmagan Paylov to'lovlarini topish uchun.
// Faqat oxirgi 24 soatnikini olamiz — eski tashlab ketilgan so'rovlarni emas.
async function listPendingPaylovTopups(limit = 50) {
    const { rows } = await pool.query(
        `SELECT * FROM balance_topups
         WHERE status = 'pending' AND paylov_order_id IS NOT NULL
           AND created_at > now() - interval '24 hours'
         ORDER BY created_at ASC LIMIT $1`,
        [limit]
    );
    return rows;
}

async function listAllTelegramIds() {
    const { rows } = await pool.query(
        `SELECT telegram_id FROM users WHERE telegram_id IS NOT NULL AND blocked = FALSE`
    );
    return rows.map((r) => r.telegram_id);
}

module.exports = {
    createPaylovTopup,
    getTopupByOrderId,
    listPendingPaylovTopups,
    isActive,
    getUserByUsername,
    getUserById,
    getUserByTelegramId,
    randomPassword,
    createUser,
    grantVip,
    revokeVip,
    blockUser,
    touchHeartbeat,
    listUsers,
    getStats,
    addBalance,
    createTopupRequest,
    confirmOldestPendingTopup,
    purchaseVip,
    listPendingTopups,
    confirmTopupById,
    listAllTelegramIds,
};
