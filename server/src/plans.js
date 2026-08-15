// VIP plans and balance top-up presets. Prices are in whole so'm.

const VIP_PLANS = [
    { key: '1d',   days: 1,   price: 8000,   label: '1 kun' },
    { key: '7d',   days: 7,   price: 40000,  label: '7 kun' },
    { key: '15d',  days: 15,  price: 50000,  label: '15 kun' },
    { key: '30d',  days: 30,  price: 99000,  label: '30 kun' },
    { key: '90d',  days: 90,  price: 230000, label: '90 kun' },
    { key: '365d', days: 365, price: 500000, label: '1 yil' },
];

const TOPUP_PRESETS = [50000, 100000, 200000, 500000];

function findPlan(key) {
    return VIP_PLANS.find((p) => p.key === key) || null;
}

function fmtSom(n) {
    return Number(n).toLocaleString('uz-UZ').replace(/,/g, ' ') + " so'm";
}

module.exports = { VIP_PLANS, TOPUP_PRESETS, findPlan, fmtSom };
