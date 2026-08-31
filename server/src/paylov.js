// Paylov (WLCM / Octagram) — Click / Payme / Uzum / karta to'lovlari.
//
// Node 18+ global fetch ishlatiladi, qo'shimcha paket kerak emas.
// HMAC imzo: METHOD \n canonical_path \n timestamp_ms \n sha256hex(body)

const crypto = require('crypto');

const BASE_URL = (process.env.WLCM_BASE_URL || 'https://api.wlcm.uz').replace(/\/$/, '');
const API_KEY = process.env.WLCM_API_KEY || '';
const API_SECRET = process.env.WLCM_API_SECRET || '';
const RETURN_URL = process.env.PAYLOV_RETURN_URL || '';

// Paylov qabul qiladigan provayderlar (kichik harflarda bo'lishi SHART)
// DIQQAT: karta (Uzcard/Humo) uchun kalit 'paylov' — 'card' EMAS.
// 'card' Paylov tomonidan qabul qilinmaydi (Provider is not configured).
const ALL_PROVIDERS = [
    { key: 'payme', label: '💳 Payme' },
    { key: 'click', label: '🔵 Click' },
    { key: 'uzum', label: '🟣 Uzum' },
    { key: 'paylov', label: '💳 Karta (Uzcard/Humo)' },
];

// Har bir provayder Octagram tomonidan alohida yoqiladi. Yoqilmaganini
// ko'rsatsak — user bosadi va xato oladi. Shuning uchun ro'yxat .env dan
// boshqariladi: PAYLOV_PROVIDERS=payme,click,uzum
// Bo'sh qoldirilsa — hammasi ko'rsatiladi.
const ENABLED = (process.env.PAYLOV_PROVIDERS || '')
    .split(',').map((s) => s.trim()).filter(Boolean);
const PROVIDERS = ENABLED.length
    ? ALL_PROVIDERS.filter((p) => ENABLED.includes(p.key))
    : ALL_PROVIDERS;

function isConfigured() {
    return Boolean(API_KEY && API_SECRET);
}

// query parametrlari alfavit bo'yicha saralanadi
function canonical(path, query = '') {
    if (!query) return path;
    const params = [...new URLSearchParams(query).entries()].sort(([a], [b]) => (a < b ? -1 : a > b ? 1 : 0));
    const enc = new URLSearchParams(params).toString();
    return enc ? `${path}?${enc}` : path;
}

function signedHeaders(method, path, body = '', query = '') {
    const ts = String(Date.now()); // millisekundda
    const bodyHash = crypto.createHash('sha256').update(body).digest('hex');
    const msg = `${method.toUpperCase()}\n${canonical(path, query)}\n${ts}\n${bodyHash}`;
    const sig = crypto.createHmac('sha256', API_SECRET).update(msg).digest('hex');
    return {
        'X-API-Key': API_KEY,
        'X-Timestamp': ts,
        'X-Signature': sig,
        'Content-Type': 'application/json',
        // Ba'zi WAF boshqa User-Agent'larni bloklaydi
        'User-Agent': 'curl/7.88.1',
    };
}

async function req(method, path, { body = null, auth = true, timeout = 30000 } = {}) {
    const raw = body ? JSON.stringify(body) : '';
    const headers = auth ? signedHeaders(method, path, raw) : { 'Content-Type': 'application/json' };
    const ctrl = new AbortController();
    const timer = setTimeout(() => ctrl.abort(), timeout);
    try {
        const r = await fetch(`${BASE_URL}${path}`, {
            method, headers, signal: ctrl.signal,
            ...(raw ? { body: raw } : {}),
        });
        const text = await r.text();
        let data = null;
        try { data = JSON.parse(text); } catch { data = text; }
        return { status: r.status, data };
    } finally {
        clearTimeout(timer);
    }
}

/**
 * To'lov havolasini yaratadi.
 * @param externalId bizning tomondagi NOYOB id
 * @param amountSom  SO'MDA (ichkarida tiyinga o'giriladi)
 * @returns {order_id, checkout_url, ...} yoki null
 */
async function createCheckout(externalId, amountSom, provider = 'payme') {
    if (!isConfigured()) {
        console.error('[paylov] WLCM_API_KEY / WLCM_API_SECRET sozlanmagan');
        return null;
    }
    try {
        const { status, data } = await req('POST', '/api/v1/integrations/checkout', {
            body: {
                external_id: externalId,
                amount: Math.round(amountSom) * 100, // MUHIM: tiyinda
                payment_provider: provider,
                return_url: RETURN_URL,
            },
        });
        if (status === 200) return data;
        // 400 "Provider is not configured" -> provayder partner akkauntga yoqilmagan
        console.error('[paylov] checkout xato', status, JSON.stringify(data).slice(0, 300));
        return null;
    } catch (e) {
        console.error('[paylov] createCheckout xato:', e.message);
        return null;
    }
}

/** Buyurtma holati — bu endpoint auth talab qilmaydi. */
async function getOrderStatus(orderId) {
    try {
        const { status, data } = await req('GET', `/api/v1/orders/${orderId}/status`, { auth: false, timeout: 15000 });
        return status === 200 ? data : null;
    } catch (e) {
        console.error('[paylov] getOrderStatus xato:', e.message);
        return null;
    }
}

/** To'landimi? (is_paid=true YOKI state=2) */
async function isPaid(orderId) {
    const st = await getOrderStatus(orderId);
    return Boolean(st && (st.is_paid === true || st.state === 2));
}

/**
 * Kalitlar to'g'rimi. ESLATMA: Paylov tomonida /partners/me ba'zan ichki 500
 * qaytaradi, LEKIN to'lov ishlayveradi — shuning uchun 5xx ni "o'lik" demaymiz.
 */
async function healthcheck() {
    if (!isConfigured()) return { ok: false, msg: 'kalit yo\'q' };
    try {
        const { status, data } = await req('GET', '/api/v1/partners/me', { timeout: 15000 });
        if (status === 200) return { ok: true, msg: 'OK' };
        const raw = typeof data === 'string' ? data : JSON.stringify(data);
        if (status >= 500 && /SERVER_ERROR|internal_server_error/.test(raw))
            return { ok: true, msg: "to'lov OK (partners/me ularning tomonida buzuq)" };
        return { ok: false, msg: `HTTP ${status}: ${raw.slice(0, 80)}` };
    } catch (e) {
        return { ok: false, msg: `${e.name}: ${e.message || 'javob bermadi'}` };
    }
}

module.exports = { PROVIDERS, ALL_PROVIDERS, isConfigured, createCheckout, getOrderStatus, isPaid, healthcheck };
