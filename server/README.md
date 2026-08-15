# kaelserver

KAEL_CHEAT uchun litsenziya API + sotuv/admin Telegram bot. VIP-only model — free tier yo'q, faol VIP granti bo'lmagan hisob login qila olmaydi.

## Tuzilishi
- `src/db.js` — Postgres ulanishi va sxema (`users`, `payments`)
- `src/auth.js` — parol hash (bcrypt) va JWT token
- `src/licenses.js` — hisob/VIP boshqaruv mantig'i (server ham, bot ham shundan foydalanadi)
- `src/server.js` — REST API (C++ klient shu bilan gaplashadi)
- `src/bot.js` — Telegram bot (mijoz + admin buyruqlari)

## Railway'da ishga tushirish

1. Railway'da yangi loyiha yarating, shu papkani (yoki git repo'sini) ulang.
2. **Postgres** plaginini qo'shing (Railway avtomatik `DATABASE_URL` beradi).
3. Loyiha sozlamalarida quyidagi environment variable'larni qo'ying (`.env.example` ga qarang):
   - `JWT_SECRET` — uzun tasodifiy satr (masalan `openssl rand -hex 32`)
   - `BOT_TOKEN` — @BotFather'dan olingan bot tokeni
   - `ADMIN_TELEGRAM_IDS` — sizning Telegram raqamli ID'ingiz (masalan @userinfobot orqali oling)
4. Deploy qiling. Railway sizga `https://<nom>.up.railway.app` manzilini beradi.
5. Shu manzilni menga ayting — C++ klientdagi `m_strApiUrl` ni shunga o'zgartiraman.

## Bot buyruqlari

**Mijoz uchun:**
- `/start` — tanishtirish
- `/buy` — VIP sotib olish (hozircha: adminga murojaat, to'lov ulangach avtomatlashadi)
- `/status <username>` — hisob holatini tekshirish

**Admin uchun** (faqat `ADMIN_TELEGRAM_IDS` dagi ID'lar):
- `/newuser <username> <kunlar>` — yangi hisob yaratadi, tasodifiy parol beradi, VIP'ni yoqadi
- `/deliver <telegram_id> <username> <parol>` — hisob ma'lumotlarini xaridorga to'g'ridan-to'g'ri yuboradi
- `/grant <username> <kunlar>` — mavjud hisobga VIP qo'shadi/uzaytiradi
- `/revoke <username>` — VIP'ni bekor qiladi
- `/block <username>` / `/unblock <username>`
- `/users` — oxirgi 30 ta hisob ro'yxati
- `/stats` — umumiy statistika

## paylov.uz integratsiyasi — HALI ULANMAGAN

`src/server.js` dagi `/api/payment/webhook` va `src/bot.js` dagi `/buy` buyrug'i **placeholder**. paylov.uz'da merchant hisob ochib, API hujjatlarini/kalitlarini olganingizdan keyin:

1. `/buy` buyrug'ini paylov.uz to'lov havolasi yaratadigan qilib o'zgartiramiz.
2. `/api/payment/webhook` dagi umumiy maxfiy kalit tekshiruvini paylov.uz'ning haqiqiy imzo tekshiruviga almashtiramiz.
3. To'lov muvaffaqiyatli bo'lganda avtomatik hisob yaratib, xaridorga Telegram orqali yuboramiz (hozir buni admin `/newuser` + `/deliver` bilan qo'lda qiladi).

## Lokal ishga tushirish (test uchun)

```
npm install
cp .env.example .env   # va qiymatlarni to'ldiring
npm start
```
