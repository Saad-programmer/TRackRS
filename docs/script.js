const menuButton = document.querySelector('.menu-toggle');
const siteNav = document.querySelector('.site-nav');

menuButton?.addEventListener('click', () => {
  const isOpen = menuButton.getAttribute('aria-expanded') === 'true';
  menuButton.setAttribute('aria-expanded', String(!isOpen));
  menuButton.setAttribute('aria-label', isOpen ? 'Open navigation' : 'Close navigation');
  siteNav?.classList.toggle('is-open', !isOpen);
});

siteNav?.querySelectorAll('a').forEach((link) => {
  link.addEventListener('click', () => {
    menuButton?.setAttribute('aria-expanded', 'false');
    menuButton?.setAttribute('aria-label', 'Open navigation');
    siteNav.classList.remove('is-open');
  });
});

const garageVideo = document.querySelector('.garage-film');
const filmToggle = document.querySelector('.film-toggle');

filmToggle?.addEventListener('click', async () => {
  if (!garageVideo) return;
  if (garageVideo.paused) {
    try {
      await garageVideo.play();
      filmToggle.setAttribute('aria-label', 'Pause garage footage');
      filmToggle.querySelector('span').textContent = 'Ⅱ';
    } catch {
      filmToggle.setAttribute('aria-label', 'Garage footage could not be played');
    }
  } else {
    garageVideo.pause();
    filmToggle.setAttribute('aria-label', 'Play garage footage');
    filmToggle.querySelector('span').textContent = '▶';
  }
});

const revealItems = document.querySelectorAll('.feature-item, .world-card, .person, .team-note, .download-panel, .key-table');
const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

if ('IntersectionObserver' in window && !reducedMotion) {
  const revealObserver = new IntersectionObserver((entries, observer) => {
    entries.forEach((entry) => {
      if (entry.isIntersecting) {
        entry.target.classList.add('is-visible');
        observer.unobserve(entry.target);
      }
    });
  }, { threshold: 0.12 });

  revealItems.forEach((item) => {
    item.setAttribute('data-reveal', '');
    revealObserver.observe(item);
  });
}