// Add simple glitch effect on title load
document.addEventListener('DOMContentLoaded', () => {
    const glitchTitle = document.querySelector('.glitch-text');
    
    // Smooth scrolling for navigation
    document.querySelectorAll('a[href^="#"]').forEach(anchor => {
        anchor.addEventListener('click', function (e) {
            e.preventDefault();
            document.querySelector(this.getAttribute('href')).scrollIntoView({
                behavior: 'smooth'
            });
        });
    });

    // Animate mock GUI sidebar clicks
    const mItems = document.querySelectorAll('.m-item');
    mItems.forEach(item => {
        item.addEventListener('click', () => {
            mItems.forEach(i => i.classList.remove('active'));
            item.classList.add('active');
            
            // tiny animation on main content
            const mainContent = document.querySelector('.mock-main');
            mainContent.style.opacity = '0';
            setTimeout(() => {
                mainContent.style.opacity = '1';
                mainContent.style.transition = 'opacity 0.3s ease';
            }, 100);
        });
    });
});

function triggerDownload(e) {
    // The actual downloading of kael_cheat.exe will be handled by the browser 
    // because of the "download" attribute and the link pointing to the .exe.
    
    // We can show a nice alert for free users.
    if(e.currentTarget.classList.contains('btn-outline')) {
        alert("Siz Dasturning BEPUL versiyasini ko'chirmoqdasiz. Pro darajasini sinab ko'rish uchun admin bilan bog'laning!");
    } else {
        alert("Dastur ko'chirilmoqda. Siz o'zingiz tanlagan rejim uchun maxsus tayyorlangan EXE faylni olyapsiz.");
    }
}
