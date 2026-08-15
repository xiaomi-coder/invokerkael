const crypto = require('crypto');
const { pool } = require('./db');
const { hashPassword } = require('./auth');

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

module.exports = {
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
};
