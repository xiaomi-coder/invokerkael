const { Telegraf, Markup } = require('telegraf');
const licenses = require('./licenses');
const { VIP_PLANS, TOPUP_PRESETS, findPlan, fmtSom } = require('./plans');

const bot = new Telegraf(process.env.BOT_TOKEN);

const ADMIN_IDS = (process.env.ADMIN_TELEGRAM_IDS || '')
    .split(',')
    .map((s) => s.trim())
    .filter(Boolean)
    .map(Number);

// Telegram user IDs currently expected to reply with a custom top-up amount.
const awaitingCustomAmount = new Set();

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
//  Keyboards
// -----------------------------------------------------------------------
function mainMenu() {
    return Markup.inlineKeyboard([
        [Markup.button.callback('👤 Profil', 'menu:profile')],
        [Markup.button.callback('💎 VIP sotib olish', 'menu:buy')],
        [Markup.button.callback("💰 Balansni to'ldirish", 'menu:topup')],
    ]);
}

function buyMenu() {
    const rows = VIP_PLANS.map((p) => [
        Markup.button.callback(`${p.label} — ${fmtSom(p.price)}`, `buy:${p.key}`),
    ]);
    rows.push([Markup.button.callback('⬅️ Orqaga', 'menu:home')]);
    return Markup.inlineKeyboard(rows);
}

function topupMenu() {
    const rows = TOPUP_PRESETS.map((amount) => [
        Markup.button.callback(fmtSom(amount), `topup:${amount}`),
    ]);
    rows.push([Markup.button.callback("✏️ Boshqa summa", 'topup:custom')]);
    rows.push([Markup.button.callback('⬅️ Orqaga', 'menu:home')]);
    return Markup.inlineKeyboard(rows);
}

// -----------------------------------------------------------------------
//  Auto-registration — every Telegram user gets a KAEL CS2 account the
//  first time they open the bot, no separate "register" step.
// -----------------------------------------------------------------------
async function ensureUser(ctx) {
    let user = await licenses.getUserByTelegramId(ctx.from.id);
    if (user) return { user, isNew: false };

    const username = `tg_${ctx.from.id}`;
    const password = licenses.randomPassword();
    user = await licenses.createUser({ username, password, telegramId: ctx.from.id });
    return { user, isNew: true, password };
}

async function sendHome(ctx, edit = false) {
    const { user, isNew, password } = await ensureUser(ctx);

    if (isNew) {
        await ctx.reply(
            "KaeL CS2 ga xush kelibsiz!\n\n" +
            "Sizga hisob yaratildi — dasturga kirish uchun shu ma'lumotlarni ishlating:\n\n" +
            `Username: ${user.username}\n` +
            `Parol: ${password}\n\n` +
            "Buni saqlab qo'ying, keyin qayta ko'rsatilmaydi."
        );
    }

    const text =
        `KaeL CS2\n\n` +
        `Username: ${user.username}\n` +
        `Balans: ${fmtSom(user.balance_som)}\n` +
        `Holat: ${fmtStatus(user)}`;

    if (edit) {
        try { await ctx.editMessageText(text, mainMenu()); return; } catch { /* fall through */ }
    }
    await ctx.reply(text, mainMenu());
}

// -----------------------------------------------------------------------
//  Customer flow
// -----------------------------------------------------------------------
bot.start((ctx) => sendHome(ctx));

bot.action('menu:home', async (ctx) => {
    await ctx.answerCbQuery();
    await sendHome(ctx, true);
});

bot.action('menu:profile', async (ctx) => {
    await ctx.answerCbQuery();
    const { user } = await ensureUser(ctx);
    const text =
        `👤 Profil\n\n` +
        `Username: ${user.username}\n` +
        `Balans: ${fmtSom(user.balance_som)}\n` +
        `Holat: ${fmtStatus(user)}\n` +
        `Ro'yxatdan o'tgan: ${new Date(user.created_at).toLocaleDateString('uz-UZ')}`;
    await ctx.editMessageText(text, Markup.inlineKeyboard([[Markup.button.callback('⬅️ Orqaga', 'menu:home')]]));
});

bot.action('menu:buy', async (ctx) => {
    await ctx.answerCbQuery();
    await ctx.editMessageText("VIP muddatini tanlang:", buyMenu());
});

bot.action(/^buy:(.+)$/, async (ctx) => {
    await ctx.answerCbQuery();
    const key = ctx.match[1];
    const plan = findPlan(key);
    if (!plan) return;

    const { user } = await ensureUser(ctx);
    const result = await licenses.purchaseVip(user.id, key);

    if (result.ok) {
        await ctx.editMessageText(
            `✅ VIP faollashtirildi!\n\nReja: ${plan.label}\nTugaydi: ${fmtExpiry(result.user)}\nQolgan balans: ${fmtSom(result.user.balance_som)}`,
            Markup.inlineKeyboard([[Markup.button.callback('⬅️ Bosh menyu', 'menu:home')]])
        );
    } else if (result.error === 'insufficient_balance') {
        await ctx.editMessageText(
            `❌ Balansda yetarli mablag' yo'q.\n\nKerak: ${fmtSom(plan.price)}\nYetishmayapti: ${fmtSom(result.needed)}\n\nBalansni to'ldiring:`,
            topupMenu()
        );
    } else {
        await ctx.editMessageText(`❌ Xato: ${result.error}`, mainMenu());
    }
});

bot.action('menu:topup', async (ctx) => {
    await ctx.answerCbQuery();
    awaitingCustomAmount.delete(ctx.from.id);
    await ctx.editMessageText("Qancha to'ldirmoqchisiz?", topupMenu());
});

async function requestTopup(ctx, amount) {
    const { user } = await ensureUser(ctx);
    await licenses.createTopupRequest(user.id, ctx.from.id, amount);

    const uname = ctx.from.username ? `@${ctx.from.username}` : `id:${ctx.from.id}`;
    for (const adminId of ADMIN_IDS) {
        try {
            await bot.telegram.sendMessage(
                adminId,
                `🔔 Yangi to'ldirish so'rovi\n\n` +
                `Foydalanuvchi: ${user.username} (${uname})\n` +
                `Summa: ${fmtSom(amount)}\n\n` +
                `Tasdiqlash uchun:\n/addbalance ${user.username} ${amount}`
            );
        } catch { /* admin bilan bot orasida chat ochilmagan bo'lishi mumkin */ }
    }

    return ctx.reply(
        `So'rovingiz qabul qilindi: ${fmtSom(amount)}.\n\n` +
        `To'lovni amalga oshiring va admin tasdiqlashini kuting — balansingiz avtomatik yangilanadi.`,
        mainMenu()
    );
}

bot.action(/^topup:(\d+)$/, async (ctx) => {
    await ctx.answerCbQuery();
    await requestTopup(ctx, Number(ctx.match[1]));
});

bot.action('topup:custom', async (ctx) => {
    await ctx.answerCbQuery();
    awaitingCustomAmount.add(ctx.from.id);
    await ctx.editMessageText("Summani so'mda yozing (masalan: 75000):");
});

bot.on('text', async (ctx, next) => {
    if (!awaitingCustomAmount.has(ctx.from.id)) return next();
    awaitingCustomAmount.delete(ctx.from.id);

    const amount = parseInt(ctx.message.text.replace(/\D/g, ''), 10);
    if (!amount || amount < 1000) {
        return ctx.reply("Noto'g'ri summa. Faqat son kiriting (masalan: 75000).", mainMenu());
    }
    return requestTopup(ctx, amount);
});

// -----------------------------------------------------------------------
//  Admin commands — restricted to ADMIN_TELEGRAM_IDS
// -----------------------------------------------------------------------
bot.command('newuser', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const parts = ctx.message.text.split(' ').slice(1);
    const [username, daysStr] = parts;
    const days = Number(daysStr);

    if (!username || !days || days <= 0)
        return ctx.reply('Foydalanish: /newuser <username> <kunlar_soni>');

    const existing = await licenses.getUserByUsername(username);
    if (existing) return ctx.reply('Bu username allaqachon mavjud.');

    const password = licenses.randomPassword();
    await licenses.createUser({ username, password });
    const user = await licenses.grantVip(username, days);

    ctx.reply(
        `Yangi hisob yaratildi:\n\n` +
        `Username: ${username}\n` +
        `Parol: ${password}\n` +
        `VIP tugaydi: ${fmtExpiry(user)}\n\n` +
        `Bu ma'lumotni xaridorga qo'lda yuboring, yoki:\n` +
        `/deliver <telegram_id> ${username} ${password}`
    );
});

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

bot.command('grant', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const [username, daysStr] = ctx.message.text.split(' ').slice(1);
    const days = Number(daysStr);
    if (!username || !days || days <= 0)
        return ctx.reply('Foydalanish: /grant <username> <kunlar_soni>');

    const user = await licenses.grantVip(username, days);
    if (!user) return ctx.reply('Bunday username topilmadi.');

    ctx.reply(`${username} uchun VIP uzaytirildi. Tugaydi: ${fmtExpiry(user)}`);
});

bot.command('revoke', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const username = ctx.message.text.split(' ').slice(1).join(' ').trim();
    if (!username) return ctx.reply('Foydalanish: /revoke <username>');

    const user = await licenses.revokeVip(username);
    if (!user) return ctx.reply('Bunday username topilmadi.');

    ctx.reply(`${username} uchun VIP bekor qilindi.`);
});

bot.command('block', async (ctx) => {
    if (!isAdmin(ctx)) return;
    const username = ctx.message.text.split(' ').slice(1).join(' ').trim();
    if (!username) return ctx.reply('Foydalanish: /block <username>');
    const user = await licenses.blockUser(username, true);
    if (!user) return ctx.reply('Bunday username topilmadi.');
    ctx.reply(`${username} bloklandi.`);
});

bot.command('unblock', async (ctx) => {
    if (!isAdmin(ctx)) return;
    const username = ctx.message.text.split(' ').slice(1).join(' ').trim();
    if (!username) return ctx.reply('Foydalanish: /unblock <username>');
    const user = await licenses.blockUser(username, false);
    if (!user) return ctx.reply('Bunday username topilmadi.');
    ctx.reply(`${username} blokdan chiqarildi.`);
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

bot.command('users', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const users = await licenses.listUsers(30);
    if (users.length === 0) return ctx.reply("Hozircha hech kim yo'q.");

    const lines = users.map((u) => `${u.username} — ${fmtStatus(u)} — balans: ${fmtSom(u.balance_som)}`);
    ctx.reply(lines.join('\n'));
});

bot.command('stats', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const s = await licenses.getStats();
    ctx.reply(
        `Jami foydalanuvchi: ${s.total_users}\n` +
        `Faol VIP: ${s.active_vip}\n` +
        `Bloklangan: ${s.blocked_users}`
    );
});

function launch() {
    bot.launch();
    console.log('[kaelserver] telegram bot started');
    process.once('SIGINT', () => bot.stop('SIGINT'));
    process.once('SIGTERM', () => bot.stop('SIGTERM'));
}

module.exports = { bot, launch };
