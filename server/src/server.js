require('dotenv').config();

const express = require('express');
const { initSchema } = require('./db');
const { verifyPassword, signToken, requireAuth } = require('./auth');
const licenses = require('./licenses');

const app = express();
app.use(express.json());

// ---------------------------------------------------------------------
//  Health check
// ---------------------------------------------------------------------
app.get('/', (req, res) => {
    res.status(200).send('KAEL server ishlayapti');
});

// ---------------------------------------------------------------------
//  POST /api/auth/login  { username, password } -> { token, user }
//
//  VIP-only model: a login only succeeds if the account exists, the
//  password matches, AND the account currently has an active (paid,
//  non-expired, non-blocked) VIP grant. There is no free tier — an
//  account with no active grant simply can't log in.
// ---------------------------------------------------------------------
app.post('/api/auth/login', async (req, res) => {
    const { username, password } = req.body || {};
    if (!username || !password)
        return res.status(400).json({ error: "Username yoki password kiritilmagan" });

    const user = await licenses.getUserByUsername(username);
    if (!user || !(await verifyPassword(password, user.password_hash)))
        return res.status(401).json({ error: "Username yoki password noto'g'ri" });

    if (user.blocked)
        return res.status(403).json({ error: "Akkaunt bloklangan" });

    if (!licenses.isActive(user))
        return res.status(403).json({ error: "VIP litsenziya faol emas" });

    const token = signToken(user);
    res.status(200).json({
        token,
        user: {
            username: user.username,
            tier: 'pro',
            expires_at: user.expires_at,
        },
    });
});

// ---------------------------------------------------------------------
//  GET /api/license/check  (Authorization: Bearer <token>)
// ---------------------------------------------------------------------
app.get('/api/license/check', requireAuth, async (req, res) => {
    const user = await licenses.getUserById(req.userId);
    if (!user) return res.status(401).json({ error: 'Token yaroqsiz' });

    if (user.blocked)
        return res.status(200).json({ valid: false, reason: 'blocked' });

    if (!licenses.isActive(user))
        return res.status(200).json({ valid: false, reason: 'expired' });

    res.status(200).json({ valid: true, tier: 'pro', expires_at: user.expires_at });
});

// ---------------------------------------------------------------------
//  POST /api/license/heartbeat  (Authorization: Bearer <token>)
// ---------------------------------------------------------------------
app.post('/api/license/heartbeat', requireAuth, async (req, res) => {
    await licenses.touchHeartbeat(req.userId);
    res.status(200).json({ ok: true });
});

// ---------------------------------------------------------------------
//  Optional dependency-file endpoints (kept for client compatibility;
//  nothing is served yet — add rows/files here if that feature is used).
// ---------------------------------------------------------------------
app.get('/api/files/list', requireAuth, (req, res) => {
    res.status(200).json({ files: [] });
});

app.get('/api/files/weapons/:filename', requireAuth, (req, res) => {
    res.status(404).json({ error: 'Fayl topilmadi' });
});

// ---------------------------------------------------------------------
//  POST /api/payment/webhook
//
//  PLACEHOLDER — paylov.uz merchant account is not set up yet. Once you
//  have their docs, replace the shared-secret header check below with
//  their real signature verification, and map their payload fields
//  (amount / invoice id / status) onto the fields read here.
// ---------------------------------------------------------------------
app.post('/api/payment/webhook', express.json(), async (req, res) => {
    const secret = req.headers['x-webhook-secret'];
    if (!process.env.PAYLOV_WEBHOOK_SECRET || secret !== process.env.PAYLOV_WEBHOOK_SECRET) {
        return res.status(401).json({ error: 'Invalid webhook secret' });
    }

    // TODO(paylov): replace with the real field names from their callback payload.
    const { username, plan_days, provider_ref, amount_tiyin } = req.body || {};
    if (!username || !plan_days) return res.status(400).json({ error: 'Missing fields' });

    const user = await licenses.grantVip(username, Number(plan_days));
    if (!user) return res.status(404).json({ error: 'User not found' });

    res.status(200).json({ ok: true, expires_at: user.expires_at });
});

// ---------------------------------------------------------------------
async function main() {
    await initSchema();
    const port = process.env.PORT || 3000;
    const httpServer = app.listen(port, () => console.log(`[kaelserver] listening on :${port}`));

    // Start the Telegram bot in the same process unless explicitly disabled.
    let botInstance = null;
    if (process.env.BOT_TOKEN) {
        botInstance = require('./bot').bot;
        require('./bot').launch();
    } else {
        console.log('[kaelserver] BOT_TOKEN not set — Telegram bot disabled');
    }

    // Single place that owns process shutdown — stopping the bot alone
    // (as Telegraf's own signal handlers do) never exits the process,
    // since the HTTP server keeps the event loop alive. systemd relies on
    // a real exit here to restart/stop us cleanly instead of escalating
    // to SIGKILL.
    const shutdown = (signal) => {
        console.log(`[kaelserver] ${signal} received, shutting down...`);
        if (botInstance) botInstance.stop(signal);
        httpServer.close(() => process.exit(0));
        setTimeout(() => process.exit(0), 3000).unref();
    };
    process.once('SIGINT', () => shutdown('SIGINT'));
    process.once('SIGTERM', () => shutdown('SIGTERM'));
}

main().catch((err) => {
    console.error('[kaelserver] fatal startup error:', err);
    process.exit(1);
});
