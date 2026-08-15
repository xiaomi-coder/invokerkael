const { Pool } = require('pg');

const pool = new Pool({
    connectionString: process.env.DATABASE_URL,
    ssl: process.env.DATABASE_URL && process.env.DATABASE_URL.includes('railway')
        ? { rejectUnauthorized: false }
        : false,
});

// VIP-only model: a user either has an active (non-expired, non-blocked) VIP
// grant or they don't — there is no free tier. Users hold a so'm balance
// (topped up manually by the admin until paylov.uz is wired) and spend it
// on VIP plans.
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
    total_minutes  INTEGER NOT NULL DEFAULT 0,
    balance_som    BIGINT NOT NULL DEFAULT 0
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

-- Balance top-up requests raised from the bot's buttons. Until paylov.uz is
-- wired, these sit 'pending' and the admin confirms them by hand with
-- /addbalance, which marks the newest matching row 'confirmed'.
CREATE TABLE IF NOT EXISTS balance_topups (
    id           SERIAL PRIMARY KEY,
    user_id      INTEGER REFERENCES users(id),
    telegram_id  BIGINT,
    amount_som   BIGINT NOT NULL,
    status       TEXT NOT NULL DEFAULT 'pending',
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    confirmed_at TIMESTAMPTZ
);

CREATE INDEX IF NOT EXISTS idx_users_telegram_id ON users(telegram_id);
CREATE INDEX IF NOT EXISTS idx_payments_status ON payments(status);
CREATE INDEX IF NOT EXISTS idx_topups_status ON balance_topups(status);

ALTER TABLE users ADD COLUMN IF NOT EXISTS balance_som BIGINT NOT NULL DEFAULT 0;
`;

async function initSchema() {
    await pool.query(SCHEMA);
}

module.exports = { pool, initSchema };
