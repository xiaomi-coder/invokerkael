const { Telegraf, Markup } = require('telegraf');
const licenses = require('./licenses');
const { VIP_PLANS, TOPUP_PRESETS, findPlan, fmtSom } = require('./plans');

const bot = new Telegraf(process.env.BOT_TOKEN);

const ADMIN_IDS = (process.env.ADMIN_TELEGRAM_IDS || '')
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean)
    .map(Number);

function isAdmin(ctx) {
    return ADMIN_IDS.includes(ctx.from.id);
}

function fmtExpiry(user) {
    if (!user.expires_at) return "yo'q";
    return new Date(user.expires_at).toLocaleString('uz-UZ');
}

function fmtStatus(user) {
    if (user.blocked) return '🔴 bloklangan';
    if (!licenses.isActive(user)) return '⚪ VIP faol emas';
    return `🟢 VIP faol (tugaydi: ${fmtExpiry(user)})`;
}

// -----------------------------------------------------------------------
//  Bottom (reply) keyboards — persistent navigation, not tied to one message
// -----------------------------------------------------------------------
const BTN = {
    PROFILE: '👤 Profil',
    BUY: '💎 VIP sotib olish',
    TOPUP: "💰 Balansni to'ldirish",
    CONTACT: "📩 Admin bilan bog'lanish",

    STATS: '📊 Statistika',
    USERS: '👥 Foydalanuvchilar',
    NEWUSER: '➕ Yangi hisob',
    ADDBALANCE: "💰 Balans qo'shish",
    GRANTVIP: '🎁 VIP berish',
    BLOCK: '🚫 Blok / Unblok',
    PENDING: "📬 Kutilayotgan to'lovlar",
    BROADCAST: '📢 Xabar yuborish',
};

const ALL_BUTTON_LABELS = new Set(Object.values(BTN));

const userKeyboard = Markup.keyboard([
    [BTN.PROFILE, BTN.BUY],
    [BTN.TOPUP, BTN.CONTACT],
]).resize();

const adminKeyboard = Markup.keyboard([
    [BTN.STATS, BTN.USERS],
    [BTN.NEWUSER, BTN.ADDBALANCE],
    [BTN.GRANTVIP, BTN.BLOCK],
    [BTN.PENDING, BTN.BROADCAST],
]).resize();

function buyMenu() {
    const rows = VIP_PLANS.map((p) => [
        Markup.button.callback(`${p.label} — ${fmtSom(p.price)}`, `buy:${p.key}`),
    ]);
    return Markup.inlineKeyboard(rows);
}

function topupMenu() {
    const rows = TOPUP_PRESETS.map((amount) => [
        Markup.button.callback(fmtSom(amount), `topup:${amount}`),
    ]);
    rows.push([Markup.button.callback("✏️ Boshqa summa", 'topup:custom')]);
    return Markup.inlineKeyboard(rows);
}

// -----------------------------------------------------------------------
//  Multi-step text flows (custom top-up amount, admin wizards, contact,
//  broadcast). Keyed by Telegram user id. Any incoming text is routed here
//  first; if nothing is pending, it falls through to bot.hears()/commands.
// -----------------------------------------------------------------------
const pendingInput = new Map();

function setFlow(telegramId, flow, step, data = {}) {
    pendingInput.set(telegramId, { flow, step, data });
}
function clearFlow(telegramId) {
    pendingInput.delete(telegramId);
}

// -----------------------------------------------------------------------
//  Registration — the user picks their own login and password (asked one
//  at a time via the 'register' text flow below). Returns the account if
//  it already exists; otherwise starts registration and returns null, so
//  callers can bail out of whatever they were doing.
// -----------------------------------------------------------------------
async function requireUser(ctx) {
    const user = await licenses.getUserByTelegramId(ctx.from.id);
    if (user) return user;

    setFlow(ctx.from.id, 'register', 1);
    await ctx.reply(
        "Avval ro'yxatdan o'tishingiz kerak.\n\nLogin (username) tanlang:",
        Markup.removeKeyboard()
    );
    return null;
}

async function sendHome(ctx) {
    if (isAdmin(ctx)) {
        const s = await licenses.getStats();
        await ctx.reply(
            `KaeL CS2 — Admin panel\n\n` +
            `Jami foydalanuvchi: ${s.total_users}\n` +
            `Faol VIP: ${s.active_vip}\n` +
            `Bloklangan: ${s.blocked_users}`,
            adminKeyboard
        );
        return;
    }

    const user = await requireUser(ctx);
    if (!user) return;

    await ctx.reply(
        `KaeL CS2\n\n` +
        `Login: ${user.username}\n` +
        `Balans: ${fmtSom(user.balance_som)}\n` +
        `Holat: ${fmtStatus(user)}`,
        userKeyboard
    );
}

// -----------------------------------------------------------------------
//  Customer flow
// -----------------------------------------------------------------------
bot.start((ctx) => sendHome(ctx));

bot.hears(BTN.PROFILE, async (ctx) => {
    const user = await requireUser(ctx);
    if (!user) return;
    await ctx.reply(
        `👤 Profil\n\n` +
        `Login: ${user.username}\n` +
        `Balans: ${fmtSom(user.balance_som)}\n` +
        `Holat: ${fmtStatus(user)}\n` +
        `Ro'yxatdan o'tgan: ${new Date(user.created_at).toLocaleDateString('uz-UZ')}`,
        userKeyboard
    );
});

bot.hears(BTN.BUY, async (ctx) => {
    const user = await requireUser(ctx);
    if (!user) return;
    await ctx.reply('VIP muddatini tanlang:', buyMenu());
});

bot.action(/^buy:(.+)$/, async (ctx) => {
    await ctx.answerCbQuery();
    const key = ctx.match[1];
    const plan = findPlan(key);
    if (!plan) return;

    const user = await requireUser(ctx);
    if (!user) return;
    const result = await licenses.purchaseVip(user.id, key);

    if (result.ok) {
        await ctx.editMessageText(
            `✅ VIP faollashtirildi!\n\nReja: ${plan.label}\nTugaydi: ${fmtExpiry(result.user)}\nQolgan balans: ${fmtSom(result.user.balance_som)}`
        );
    } else if (result.error === 'insufficient_balance') {
        await ctx.editMessageText(
            `❌ Balansda yetarli mablag' yo'q.\n\nKerak: ${fmtSom(plan.price)}\nYetishmayapti: ${fmtSom(result.needed)}\n\nBalansni to'ldiring:`,
            topupMenu()
        );
    } else {
        await ctx.editMessageText(`❌ Xato: ${result.error}`);
    }
});

bot.hears(BTN.TOPUP, async (ctx) => {
    const user = await requireUser(ctx);
    if (!user) return;
    clearFlow(ctx.from.id);
    await ctx.reply("Qancha to'ldirmoqchisiz?", topupMenu());
});

async function requestTopup(ctx, amount) {
    const user = await requireUser(ctx);
    if (!user) return;
    await licenses.createTopupRequest(user.id, ctx.from.id, amount);

    const uname = ctx.from.username ? `@${ctx.from.username}` : `id:${ctx.from.id}`;
    for (const adminId of ADMIN_IDS) {
        try {
            await bot.telegram.sendMessage(
                adminId,
                `🔔 Yangi to'ldirish so'rovi\n\n` +
                `Foydalanuvchi: ${user.username} (${uname})\n` +
                `Summa: ${fmtSom(amount)}\n\n` +
                `"${BTN.PENDING}" bo'limidan tasdiqlang, yoki:\n/addbalance ${user.username} ${amount}`
            );
        } catch { /* admin bilan bot orasida chat ochilmagan bo'lishi mumkin */ }
    }

    return ctx.reply(
        `So'rovingiz qabul qilindi: ${fmtSom(amount)}.\n\n` +
        `To'lovni amalga oshiring va admin tasdiqlashini kuting — balansingiz avtomatik yangilanadi.`,
        userKeyboard
    );
}

bot.action(/^topup:(\d+)$/, async (ctx) => {
    await ctx.answerCbQuery();
    await requestTopup(ctx, Number(ctx.match[1]));
});

bot.action('topup:custom', async (ctx) => {
    await ctx.answerCbQuery();
    setFlow(ctx.from.id, 'topup_custom', 1);
    await ctx.editMessageText("Summani so'mda yozing (masalan: 75000):");
});

bot.hears(BTN.CONTACT, async (ctx) => {
    const user = await requireUser(ctx);
    if (!user) return;
    setFlow(ctx.from.id, 'contact_admin', 1);
    await ctx.reply("Xabaringizni yozing — admin uni ko'radi va sizga javob beradi:", Markup.removeKeyboard());
});

// -----------------------------------------------------------------------
//  Admin panel
// -----------------------------------------------------------------------
bot.hears(BTN.STATS, async (ctx) => {
    if (!isAdmin(ctx)) return;
    const s = await licenses.getStats();
    await ctx.reply(
        `📊 Statistika\n\n` +
        `Jami foydalanuvchi: ${s.total_users}\n` +
        `Faol VIP: ${s.active_vip}\n` +
        `Bloklangan: ${s.blocked_users}`,
        adminKeyboard
    );
});

bot.hears(BTN.USERS, async (ctx) => {
    if (!isAdmin(ctx)) return;
    const users = await licenses.listUsers(30);
    if (users.length === 0) return ctx.reply("Hozircha hech kim yo'q.", adminKeyboard);

    const lines = users.map((u) => `${u.username} — ${fmtStatus(u)} — balans: ${fmtSom(u.balance_som)}`);
    await ctx.reply(`👥 Oxirgi ${users.length} ta hisob\n\n${lines.join('\n')}`, adminKeyboard);
});

bot.hears(BTN.PENDING, async (ctx) => {
    if (!isAdmin(ctx)) return;
    const rows = await licenses.listPendingTopups(20);
    if (rows.length === 0) return ctx.reply("Kutilayotgan so'rovlar yo'q.", adminKeyboard);

    await ctx.reply(`📬 Kutilayotgan to'lovlar: ${rows.length} ta`, adminKeyboard);
    for (const r of rows) {
        await ctx.reply(
            `${r.username} — ${fmtSom(r.amount_som)}\n${new Date(r.created_at).toLocaleString('uz-UZ')}`,
            Markup.inlineKeyboard([[Markup.button.callback('✅ Tasdiqlash', `topupok:${r.id}`)]])
        );
    }
});

bot.action(/^topupok:(\d+)$/, async (ctx) => {
    if (!isAdmin(ctx)) return ctx.answerCbQuery();
    await ctx.answerCbQuery();

    const result = await licenses.confirmTopupById(Number(ctx.match[1]));
    if (!result) return ctx.editMessageText('❌ Bu so\'rov allaqachon ko\'rib chiqilgan.');

    await ctx.editMessageText(`✅ Tasdiqlandi: ${result.user.username} — ${fmtSom(result.topup.amount_som)}`);

    if (result.user.telegram_id) {
        try {
            await ctx.telegram.sendMessage(
                result.user.telegram_id,
                `✅ Balansingizga ${fmtSom(result.topup.amount_som)} qo'shildi!\nYangi balans: ${fmtSom(result.user.balance_som)}`
            );
        } catch { /* foydalanuvchi botni bloklagan bo'lishi mumkin */ }
    }
});

bot.hears(BTN.NEWUSER, (ctx) => {
    if (!isAdmin(ctx)) return;
    setFlow(ctx.from.id, 'admin_newuser', 1);
    ctx.reply('Yangi hisob uchun username kiriting:', Markup.removeKeyboard());
});

bot.hears(BTN.ADDBALANCE, (ctx) => {
    if (!isAdmin(ctx)) return;
    setFlow(ctx.from.id, 'admin_addbalance', 1);
    ctx.reply('Kimning balansiga qo\'shamiz? Username kiriting:', Markup.removeKeyboard());
});

bot.hears(BTN.GRANTVIP, (ctx) => {
    if (!isAdmin(ctx)) return;
    setFlow(ctx.from.id, 'admin_grantvip', 1);
    ctx.reply('Kimga VIP beramiz? Username kiriting:', Markup.removeKeyboard());
});

bot.hears(BTN.BLOCK, (ctx) => {
    if (!isAdmin(ctx)) return;
    setFlow(ctx.from.id, 'admin_block', 1);
    ctx.reply('Username kiriting:', Markup.removeKeyboard());
});

bot.hears(BTN.BROADCAST, (ctx) => {
    if (!isAdmin(ctx)) return;
    setFlow(ctx.from.id, 'admin_broadcast', 1);
    ctx.reply("Barcha foydalanuvchilarga yuboriladigan xabarni yozing:", Markup.removeKeyboard());
});

// -----------------------------------------------------------------------
//  Text dispatcher — routes to the active multi-step flow, if any.
// -----------------------------------------------------------------------
bot.on('text', async (ctx, next) => {
    const text = ctx.message.text.trim();
    if (text.startsWith('/') || ALL_BUTTON_LABELS.has(text)) {
        clearFlow(ctx.from.id);
        return next();
    }

    const state = pendingInput.get(ctx.from.id);
    if (!state) return next();

    switch (state.flow) {
        case 'register': {
            if (state.step === 1) {
                const login = text;
                if (login.length < 3)
                    return ctx.reply("Login juda qisqa. Kamida 3 ta belgi kiriting:");
                if (!/^[a-zA-Z0-9_]+$/.test(login))
                    return ctx.reply("Login faqat lotin harflari, raqam va pastki chiziqdan iborat bo'lsin. Qaytadan kiriting:");

                const existing = await licenses.getUserByUsername(login);
                if (existing)
                    return ctx.reply("Bu login band. Boshqa login kiriting:");

                state.data.username = login;
                state.step = 2;
                return ctx.reply("Parolni kiriting (kamida 4 ta belgi):");
            }
            if (state.step === 2) {
                const password = text;
                clearFlow(ctx.from.id);
                if (password.length < 4)
                    return ctx.reply("Parol juda qisqa. Qaytadan /start bosing.", Markup.removeKeyboard());

                await licenses.createUser({ username: state.data.username, password, telegramId: ctx.from.id });
                return ctx.reply(
                    `✅ Ro'yxatdan o'tdingiz!\n\nLogin: ${state.data.username}\n\n` +
                    `Dasturga shu login va parol bilan kirishingiz mumkin.`,
                    userKeyboard
                );
            }
            return;
        }

        case 'topup_custom': {
            clearFlow(ctx.from.id);
            const amount = parseInt(text.replace(/\D/g, ''), 10);
            if (!amount || amount < 1000) return ctx.reply("Noto'g'ri summa.", userKeyboard);
            return requestTopup(ctx, amount);
        }

        case 'contact_admin': {
            clearFlow(ctx.from.id);
            const user = await requireUser(ctx);
            if (!user) return;
            const uname = ctx.from.username ? `@${ctx.from.username}` : `id:${ctx.from.id}`;
            for (const adminId of ADMIN_IDS) {
                try {
                    await bot.telegram.sendMessage(
                        adminId,
                        `📩 Yangi xabar\n\nKimdan: ${user.username} (${uname})\n\n${text}`
                    );
                } catch { /* ignore */ }
            }
            return ctx.reply("Xabaringiz adminga yuborildi. Tez orada javob beradi.", userKeyboard);
        }

        case 'admin_newuser': {
            if (!isAdmin(ctx)) { clearFlow(ctx.from.id); return; }
            if (state.step === 1) {
                const existing = await licenses.getUserByUsername(text);
                if (existing) { clearFlow(ctx.from.id); return ctx.reply('Bu username allaqachon mavjud.', adminKeyboard); }
                state.data.username = text;
                state.step = 2;
                return ctx.reply("Necha kunlik VIP bilan? (0 = VIPsiz hisob)");
            }
            if (state.step === 2) {
                const days = Number(text);
                clearFlow(ctx.from.id);
                if (!Number.isFinite(days) || days < 0) return ctx.reply("Noto'g'ri son.", adminKeyboard);

                const password = licenses.randomPassword();
                await licenses.createUser({ username: state.data.username, password });
                const user = days > 0 ? await licenses.grantVip(state.data.username, days) : null;

                return ctx.reply(
                    `Yangi hisob yaratildi:\n\n` +
                    `Username: ${state.data.username}\n` +
                    `Parol: ${password}\n` +
                    (user ? `VIP tugaydi: ${fmtExpiry(user)}\n` : '') +
                    `\nBu ma'lumotni xaridorga qo'lda yuboring, yoki:\n/deliver <telegram_id> ${state.data.username} ${password}`,
                    adminKeyboard
                );
            }
            return;
        }

        case 'admin_addbalance': {
            if (!isAdmin(ctx)) { clearFlow(ctx.from.id); return; }
            if (state.step === 1) {
                const user = await licenses.getUserByUsername(text);
                if (!user) { clearFlow(ctx.from.id); return ctx.reply('Bunday username topilmadi.', adminKeyboard); }
                state.data.username = text;
                state.step = 2;
                return ctx.reply('Qancha summa qo\'shamiz?');
            }
            if (state.step === 2) {
                const amount = Number(text);
                clearFlow(ctx.from.id);
                if (!amount || amount <= 0) return ctx.reply("Noto'g'ri summa.", adminKeyboard);

                const user = await licenses.addBalance(state.data.username, amount);
                await licenses.confirmOldestPendingTopup(user.id);
                await ctx.reply(`${state.data.username} balansiga ${fmtSom(amount)} qo'shildi. Yangi balans: ${fmtSom(user.balance_som)}`, adminKeyboard);

                if (user.telegram_id) {
                    try {
                        await ctx.telegram.sendMessage(
                            user.telegram_id,
                            `✅ Balansingizga ${fmtSom(amount)} qo'shildi!\nYangi balans: ${fmtSom(user.balance_som)}`
                        );
                    } catch { /* ignore */ }
                }
                return;
            }
            return;
        }

        case 'admin_grantvip': {
            if (!isAdmin(ctx)) { clearFlow(ctx.from.id); return; }
            if (state.step === 1) {
                const user = await licenses.getUserByUsername(text);
                if (!user) { clearFlow(ctx.from.id); return ctx.reply('Bunday username topilmadi.', adminKeyboard); }
                state.data.username = text;
                state.step = 2;
                return ctx.reply('Necha kunlik VIP?');
            }
            if (state.step === 2) {
                const days = Number(text);
                clearFlow(ctx.from.id);
                if (!days || days <= 0) return ctx.reply("Noto'g'ri son.", adminKeyboard);

                const user = await licenses.grantVip(state.data.username, days);
                await ctx.reply(`${state.data.username} uchun VIP uzaytirildi. Tugaydi: ${fmtExpiry(user)}`, adminKeyboard);

                if (user.telegram_id) {
                    try {
                        await ctx.telegram.sendMessage(
                            user.telegram_id,
                            `🎁 Sizga ${days} kunlik VIP berildi!\nTugaydi: ${fmtExpiry(user)}`
                        );
                    } catch { /* ignore */ }
                }
                return;
            }
            return;
        }

        case 'admin_block': {
            if (!isAdmin(ctx)) { clearFlow(ctx.from.id); return; }
            clearFlow(ctx.from.id);
            const user = await licenses.getUserByUsername(text);
            if (!user) return ctx.reply('Bunday username topilmadi.', adminKeyboard);

            return ctx.reply(
                `${text} — hozirgi holat: ${fmtStatus(user)}`,
                Markup.inlineKeyboard([[
                    Markup.button.callback('🚫 Bloklash', `block:${text}`),
                    Markup.button.callback('✅ Blokdan chiqarish', `unblock:${text}`),
                ]])
            );
        }

        case 'admin_broadcast': {
            if (!isAdmin(ctx)) { clearFlow(ctx.from.id); return; }
            clearFlow(ctx.from.id);
            const ids = await licenses.listAllTelegramIds();
            let sent = 0;
            for (const id of ids) {
                try {
                    await bot.telegram.sendMessage(id, `📢 ${text}`);
                    sent++;
                } catch { /* user blocked the bot */ }
                await new Promise((r) => setTimeout(r, 35));
            }
            return ctx.reply(`Xabar yuborildi: ${sent}/${ids.length} foydalanuvchiga.`, adminKeyboard);
        }

        default:
            clearFlow(ctx.from.id);
            return next();
    }
});

bot.action(/^block:(.+)$/, async (ctx) => {
    if (!isAdmin(ctx)) return ctx.answerCbQuery();
    await ctx.answerCbQuery();
    const user = await licenses.blockUser(ctx.match[1], true);
    if (!user) return ctx.editMessageText('Bunday username topilmadi.');
    await ctx.editMessageText(`${ctx.match[1]} bloklandi.`);
});

bot.action(/^unblock:(.+)$/, async (ctx) => {
    if (!isAdmin(ctx)) return ctx.answerCbQuery();
    await ctx.answerCbQuery();
    const user = await licenses.blockUser(ctx.match[1], false);
    if (!user) return ctx.editMessageText('Bunday username topilmadi.');
    await ctx.editMessageText(`${ctx.match[1]} blokdan chiqarildi.`);
});

// -----------------------------------------------------------------------
//  Admin text commands — kept as a fallback alongside the button flows
// -----------------------------------------------------------------------
bot.command('deliver', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const [telegramIdStr, username, password] = ctx.message.text.split(' ').slice(1);
    const telegramId = Number(telegramIdStr);
    if (!telegramId || !username || !password)
        return ctx.reply('Foydalanish: /deliver <telegram_id> <username> <parol>');

    try {
        await ctx.telegram.sendMessage(
            telegramId,
            `KaeL CS2 hisobingiz tayyor!\n\nUsername: ${username}\nParol: ${password}\n\n` +
            "Dasturni oching va shu ma'lumotlar bilan kiring."
        );
        ctx.reply('Yuborildi.');
    } catch (err) {
        ctx.reply(`Yuborib bo'lmadi: ${err.message}`);
    }
});

bot.command('addbalance', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const [username, amountStr] = ctx.message.text.split(' ').slice(1);
    const amount = Number(amountStr);
    if (!username || !amount || amount <= 0)
        return ctx.reply('Foydalanish: /addbalance <username> <summa>');

    const user = await licenses.addBalance(username, amount);
    if (!user) return ctx.reply('Bunday username topilmadi.');
    await licenses.confirmOldestPendingTopup(user.id);

    ctx.reply(`${username} balansiga ${fmtSom(amount)} qo'shildi. Yangi balans: ${fmtSom(user.balance_som)}`);

    if (user.telegram_id) {
        try {
            await ctx.telegram.sendMessage(
                user.telegram_id,
                `✅ Balansingizga ${fmtSom(amount)} qo'shildi!\nYangi balans: ${fmtSom(user.balance_som)}`
            );
        } catch { /* foydalanuvchi botni bloklagan bo'lishi mumkin */ }
    }
});

function launch() {
    bot.launch();
    console.log('[kaelserver] telegram bot started');
    // Shutdown (SIGINT/SIGTERM) is handled centrally in server.js, which
    // also owns the HTTP server and needs to be the one to process.exit().
}

module.exports = { bot, launch };
