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

const mapSearch = document.querySelector('#map-search');
const mapType = document.querySelector('#map-type');
const mapList = document.querySelector('#map-list');
const mapCount = document.querySelector('#map-count');
const mapMore = document.querySelector('#map-more');
const mapError = document.querySelector('#map-error');
const pageSize = 48;
let stageCatalog = [];
let visibleStages = pageSize;

function renderStages() {
  if (!mapList || !mapCount || !mapSearch || !mapType || !mapMore) return;

  const query = mapSearch.value.trim().toLocaleLowerCase();
  const selectedType = mapType.value;
  const matches = stageCatalog.filter((stage) => {
    const matchesType = selectedType === 'all' || stage.type === selectedType;
    const matchesQuery = `${stage.name} ${stage.collection}`.toLocaleLowerCase().includes(query);
    return matchesType && matchesQuery;
  });

  mapCount.textContent = `${matches.length} of ${stageCatalog.length} stages`;
  mapList.replaceChildren();

  matches.slice(0, visibleStages).forEach((stage) => {
    const item = document.createElement('li');
    const link = document.createElement('a');
    const name = document.createElement('strong');
    const details = document.createElement('span');

    item.className = 'map-entry';
    link.href = `https://github.com/Saad-programmer/TRackRS/blob/main/${stage.path}`;
    link.target = '_blank';
    link.rel = 'noopener noreferrer';
    name.textContent = stage.name.replace(/(^|[-_])([a-z])/g, (_, separator, letter) => `${separator ? ' ' : ''}${letter.toUpperCase()}`);
    details.textContent = stage.collection;
    link.append(name, details);
    item.append(link);
    mapList.append(item);
  });

  if (matches.length === 0) {
    const empty = document.createElement('li');
    empty.className = 'catalog-empty';
    empty.textContent = 'No stages match that search.';
    mapList.append(empty);
  }

  mapMore.hidden = matches.length <= visibleStages;
}

if (mapList && mapCount && mapSearch && mapType && mapMore) {
  fetch('assets/map-catalog.json')
    .then((response) => {
      if (!response.ok) throw new Error(`Catalog request failed: ${response.status}`);
      return response.json();
    })
    .then((catalog) => {
      if (!Array.isArray(catalog.standalone) || !Array.isArray(catalog.events)) {
        throw new Error('Catalog data has an unexpected shape.');
      }

      stageCatalog = [
        ...catalog.standalone.map((stage) => ({ ...stage, type: 'standalone' })),
        ...catalog.events.map((stage) => ({ ...stage, type: 'events' })),
      ];
      renderStages();
    })
    .catch((error) => {
      mapCount.textContent = 'Stage list unavailable';
      if (mapError) mapError.hidden = false;
      console.error('Unable to load the TrackRS stage catalog.', error);
    });

  mapSearch.addEventListener('input', () => {
    visibleStages = pageSize;
    renderStages();
  });
  mapType.addEventListener('change', () => {
    visibleStages = pageSize;
    renderStages();
  });
  mapMore.addEventListener('click', () => {
    visibleStages += pageSize;
    renderStages();
  });
}

const revealItems = document.querySelectorAll('.feature-item, .world-card, .vehicle-card, .screenshot-card, .detail-note, .note-card, .person, .team-note, .download-panel, .key-table');
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