const { Pool } = require('pg');

const pool = new Pool({
    connectionString: process.env.DATABASE_URL,
    ssl: process.env.DATABASE_URL && process.env.DATABASE_URL.includes('railway')
        ? { rejectUnauthorized: false }
        : false,
});

// VIP-only model: a user either has an active (non-expired, non-blocked) VIP
// grant or they don't — there is no free tier. `plan_days` on the last
// payment is kept for admin visibility only.
const SCHEMA = `
CREATE TABLE IF NOT EXISTS users (
    id             SERIAL PRIMARY KEY,
    username       TEXT UNIQUE NOT NULL,
    password_hash  TEXT NOT NULL,
    telegram_id    BIGINT UNIQUE,
    is_vip         BOOLEAN NOT NULL DEFAULT FALSE,
    expires_at     TIMESTAMPTZ,
    blocked        BOOLEAN NOT NULL DEFAULT FALSE,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    last_seen_at   TIMESTAMPTZ,
    last_online_at TIMESTAMPTZ,
    total_minutes  INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS payments (
    id            SERIAL PRIMARY KEY,
    user_id       INTEGER REFERENCES users(id),
    telegram_id   BIGINT,
    provider      TEXT NOT NULL DEFAULT 'paylov',
    provider_ref  TEXT UNIQUE,
    amount_tiyin  BIGINT,
    plan_days     INTEGER,
    status        TEXT NOT NULL DEFAULT 'pending',
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    paid_at       TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_users_telegram_id ON users(telegram_id);
CREATE INDEX IF NOT EXISTS idx_payments_status ON payments(status);
`;

async function initSchema() {
    await pool.query(SCHEMA);
}

module.exports = { pool, initSchema };
