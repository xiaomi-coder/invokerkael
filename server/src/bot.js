const { Telegraf } = require('telegraf');
const licenses = require('./licenses');

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
    if (!user.expires_at) return 'yo\'q';
    return new Date(user.expires_at).toLocaleString('uz-UZ');
}

function fmtStatus(user) {
    if (user.blocked) return '🔴 bloklangan';
    if (!licenses.isActive(user)) return '⚪ VIP faol emas';
    return `🟢 VIP faol (tugaydi: ${fmtExpiry(user)})`;
}

// -----------------------------------------------------------------------
//  Customer commands
// -----------------------------------------------------------------------
bot.start((ctx) => {
    ctx.reply(
        'KAEL_CHEAT ga xush kelibsiz!\n\n' +
        '/buy — VIP sotib olish\n' +
        '/status <username> — hisob holatini tekshirish'
    );
});

bot.command('buy', (ctx) => {
    // TODO(paylov): once the merchant account exists, generate a real
    // paylov.uz payment link here instead of this manual-contact message.
    ctx.reply(
        'Hozircha avtomatik to\'lov ulanmagan.\n' +
        'VIP sotib olish uchun admin bilan bog\'laning, u sizga hisob yaratib beradi.'
    );
});

bot.command('status', async (ctx) => {
    const username = ctx.message.text.split(' ').slice(1).join(' ').trim();
    if (!username) return ctx.reply('Foydalanish: /status <username>');

    const user = await licenses.getUserByUsername(username);
    if (!user) return ctx.reply('Bunday username topilmadi.');

    ctx.reply(`${username}: ${fmtStatus(user)}`);
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
            `KAEL_CHEAT hisobingiz tayyor!\n\nUsername: ${username}\nParol: ${password}\n\n` +
            'Dasturni oching va shu ma\'lumotlar bilan kiring.'
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

bot.command('users', async (ctx) => {
    if (!isAdmin(ctx)) return;

    const users = await licenses.listUsers(30);
    if (users.length === 0) return ctx.reply('Hozircha hech kim yo\'q.');

    const lines = users.map((u) => `${u.username} — ${fmtStatus(u)}`);
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
