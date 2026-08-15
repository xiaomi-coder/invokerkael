const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');

const JWT_SECRET = process.env.JWT_SECRET;
if (!JWT_SECRET) {
    throw new Error('JWT_SECRET is not set — refusing to start with an insecure default.');
}

async function hashPassword(plain) {
    return bcrypt.hash(plain, 12);
}

async function verifyPassword(plain, hash) {
    return bcrypt.compare(plain, hash);
}

function signToken(user) {
    return jwt.sign(
        { sub: user.id, username: user.username },
        JWT_SECRET,
        { expiresIn: '30d' }
    );
}

function verifyToken(token) {
    try {
        return jwt.verify(token, JWT_SECRET);
    } catch {
        return null;
    }
}

// Express middleware: reads "Authorization: Bearer <token>", attaches req.userId.
function requireAuth(req, res, next) {
    const header = req.headers['authorization'] || '';
    const token = header.startsWith('Bearer ') ? header.slice(7) : null;
    if (!token) return res.status(401).json({ error: 'Token kerak' });

    const payload = verifyToken(token);
    if (!payload) return res.status(401).json({ error: 'Token yaroqsiz' });

    req.userId = payload.sub;
    next();
}

module.exports = { hashPassword, verifyPassword, signToken, verifyToken, requireAuth };
