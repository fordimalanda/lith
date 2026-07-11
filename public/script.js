/**
 * LITH Server - Dashboard Interactive Script
 * Version 1.0.9
 * 
 * Fonctionnalités :
 * - Mise à jour en temps réel des statistiques
 * - Animation des barres de progression
 * - Gestion des actions (boutons)
 * - Horloge en direct
 * - Rafraîchissement des données
 * - Nettoyage du flux d'activité
 */

(function() {
    'use strict';

    // ============================================================
    // ÉLÉMENTS DOM
    // ============================================================
    const elements = {
        statRequests: document.getElementById('statRequests'),
        statUptime: document.getElementById('statUptime'),
        statResponse: document.getElementById('statResponse'),
        statThreads: document.getElementById('statThreads'),
        cpuPercent: document.getElementById('cpuPercent'),
        memPercent: document.getElementById('memPercent'),
        diskPercent: document.getElementById('diskPercent'),
        cpuBar: document.getElementById('cpuBar'),
        memBar: document.getElementById('memBar'),
        diskBar: document.getElementById('diskBar'),
        timestamp: document.getElementById('liveTimestamp'),
        activityFeed: document.getElementById('activityFeed'),
        refreshBtn: document.getElementById('refreshBtn'),
        clearActivityBtn: document.getElementById('clearActivityBtn')
    };

    // ============================================================
    // UTILITAIRES
    // ============================================================
    function formatNumber(num) {
        return num.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ',');
    }

    function randomBetween(min, max) {
        return Math.floor(Math.random() * (max - min + 1)) + min;
    }

    function randomFloat(min, max, decimals = 1) {
        return (Math.random() * (max - min) + min).toFixed(decimals);
    }

    function getTimestamp() {
        const now = new Date();
        return now.toLocaleTimeString('fr-FR', {
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });
    }

    function getFullTimestamp() {
        const now = new Date();
        return now.toLocaleString('fr-FR', {
            day: '2-digit',
            month: 'short',
            year: 'numeric',
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });
    }

    // ============================================================
    // SIMULATION DE DONNÉES (remplace l'API)
    // ============================================================
    function generateStats() {
        const requests = randomBetween(900, 1800);
        const uptime = (99.8 + Math.random() * 0.2).toFixed(2);
        const response = randomBetween(28, 65);
        const threads = randomBetween(4, 12);
        
        const cpu = randomBetween(20, 70);
        const mem = randomBetween(40, 80);
        const disk = randomBetween(25, 60);

        return { requests, uptime, response, threads, cpu, mem, disk };
    }

    // ============================================================
    // MISES À JOUR DE L'UI
    // ============================================================
    function updateDashboard() {
        const stats = generateStats();

        // Statistiques
        if (elements.statRequests) {
            elements.statRequests.textContent = formatNumber(stats.requests);
        }
        if (elements.statUptime) {
            elements.statUptime.textContent = stats.uptime + '%';
        }
        if (elements.statResponse) {
            elements.statResponse.textContent = stats.response + 'ms';
        }
        if (elements.statThreads) {
            elements.statThreads.textContent = stats.threads;
        }

        // Ressources
        if (elements.cpuPercent) {
            elements.cpuPercent.textContent = stats.cpu + '%';
        }
        if (elements.cpuBar) {
            elements.cpuBar.style.width = stats.cpu + '%';
        }

        if (elements.memPercent) {
            elements.memPercent.textContent = stats.mem + '%';
        }
        if (elements.memBar) {
            elements.memBar.style.width = stats.mem + '%';
        }

        if (elements.diskPercent) {
            elements.diskPercent.textContent = stats.disk + '%';
        }
        if (elements.diskBar) {
            elements.diskBar.style.width = stats.disk + '%';
        }

        // Horodatage
        if (elements.timestamp) {
            elements.timestamp.textContent = getFullTimestamp();
        }
    }

    // ============================================================
    // AJOUTER UNE ACTIVITÉ
    // ============================================================
    function addActivity(method, path) {
        if (!elements.activityFeed) return;

        const methods = {
            GET: 'method-get',
            POST: 'method-post',
            DELETE: 'method-delete',
            PUT: 'method-post',
            PATCH: 'method-post'
        };

        const methodClass = methods[method] || 'method-get';
        const time = getTimestamp();

        const item = document.createElement('div');
        item.className = 'activity-item';
        item.style.opacity = '0';
        item.style.transform = 'translateY(-6px)';
        item.style.transition = 'opacity 0.3s ease, transform 0.3s ease';

        item.innerHTML = `
            <span class="activity-desc">
                <span class="method ${methodClass}">${method}</span> ${path}
            </span>
            <span class="activity-time">${time}</span>
        `;

        // Insérer en haut
        elements.activityFeed.prepend(item);

        // Animation d'entrée
        requestAnimationFrame(() => {
            item.style.opacity = '1';
            item.style.transform = 'translateY(0)';
        });

        // Limiter le nombre d'éléments
        const items = elements.activityFeed.querySelectorAll('.activity-item');
        if (items.length > 50) {
            items.forEach((el, index) => {
                if (index >= 40) {
                    el.remove();
                }
            });
        }
    }

    // ============================================================
    // SIMULATION D'ACTIVITÉS ALÉATOIRES
    // ============================================================
    function generateRandomActivity() {
        const methods = ['GET', 'POST', 'DELETE', 'GET', 'GET', 'PUT'];
        const paths = [
            '/index.html', '/style.css', '/dashboard.html',
            '/api/status', '/api/users', '/api/cache',
            '/assets/logo.svg', '/favicon.ico', '/manifest.json',
            '/api/health', '/config.json', '/robots.txt'
        ];

        const method = methods[Math.floor(Math.random() * methods.length)];
        const path = paths[Math.floor(Math.random() * paths.length)];

        addActivity(method, path);
    }

    // ============================================================
    // ACTIONS DES BOUTONS (exposées globalement)
    // ============================================================
    window.handleAction = function(action) {
        const actions = {
            restart: {
                icon: 'fa-rotate',
                label: 'Restarting server...',
                color: 'var(--accent)'
            },
            logs: {
                icon: 'fa-list-ul',
                label: 'Opening logs...',
                color: 'var(--info)'
            },
            config: {
                icon: 'fa-gear',
                label: 'Loading configuration...',
                color: 'var(--warning)'
            },
            backup: {
                icon: 'fa-database',
                label: 'Creating backup...',
                color: 'var(--purple)'
            },
            stop: {
                icon: 'fa-stop',
                label: 'Stopping server...',
                color: 'var(--danger)'
            }
        };

        const actionData = actions[action];
        if (!actionData) return;

        // Feedback visuel simple
        console.log(`[ACTION] ${actionData.label}`);

        // Ajouter une activité
        addActivity('POST', `/api/${action}`);

        // Animation sur le bouton cliqué
        const buttons = document.querySelectorAll('.quick-actions .btn');
        buttons.forEach(btn => {
            if (btn.textContent.trim().toLowerCase().includes(action)) {
                btn.style.opacity = '0.6';
                btn.style.transform = 'scale(0.97)';
                setTimeout(() => {
                    btn.style.opacity = '1';
                    btn.style.transform = 'scale(1)';
                }, 400);
            }
        });

        // Notification (simulée)
        if (action === 'stop') {
            setTimeout(() => {
                alert('⚠️ Server stopped (simulation)');
            }, 300);
        }
    };

    // ============================================================
    // RAFRAÎCHIR
    // ============================================================
    function handleRefresh() {
        if (elements.refreshBtn) {
            const icon = elements.refreshBtn.querySelector('i');
            if (icon) {
                icon.style.animation = 'spin 0.6s ease';
                setTimeout(() => {
                    icon.style.animation = '';
                }, 600);
            }
        }

        updateDashboard();
        addActivity('GET', '/api/refresh');

        // Ajouter un effet visuel sur les cartes
        document.querySelectorAll('.stat-card').forEach((card, index) => {
            card.style.transition = 'background 0.15s ease';
            card.style.background = 'rgba(16, 185, 129, 0.06)';
            setTimeout(() => {
                card.style.background = '';
            }, 200 + index * 50);
        });
    }

    // ============================================================
    // EFFACER L'ACTIVITÉ
    // ============================================================
    function clearActivity() {
        if (!elements.activityFeed) return;
        
        const items = elements.activityFeed.querySelectorAll('.activity-item');
        if (items.length === 0) return;

        items.forEach((item, index) => {
            setTimeout(() => {
                item.style.opacity = '0';
                item.style.transform = 'translateX(20px)';
                setTimeout(() => {
                    if (item.parentNode) {
                        item.remove();
                    }
                }, 300);
            }, index * 30);
        });

        // Ajouter un message après la suppression
        setTimeout(() => {
            const emptyMsg = document.createElement('div');
            emptyMsg.className = 'activity-item';
            emptyMsg.style.textAlign = 'center';
            emptyMsg.style.color = 'var(--text-muted)';
            emptyMsg.style.fontSize = '0.85rem';
            emptyMsg.style.padding = '1rem 0';
            emptyMsg.innerHTML = '<i class="fas fa-inbox"></i> No activity to display';
            elements.activityFeed.appendChild(emptyMsg);
            
            setTimeout(() => {
                if (emptyMsg.parentNode) {
                    emptyMsg.remove();
                }
            }, 2000);
        }, items.length * 30 + 200);
    }

    // ============================================================
    // ANIMATION SPIN (CSS Keyframes)
    // ============================================================
    const styleSheet = document.createElement('style');
    styleSheet.textContent = `
        @keyframes spin {
            from { transform: rotate(0deg); }
            to { transform: rotate(360deg); }
        }
    `;
    document.head.appendChild(styleSheet);

    // ============================================================
    // INITIALISATION
    // ============================================================
    function init() {
        // Mise à jour initiale
        updateDashboard();

        // Ajouter quelques activités initiales
        const initialActivities = [
            ['GET', '/index.html'],
            ['GET', '/style.css'],
            ['GET', '/dashboard.html']
        ];
        initialActivities.forEach(([method, path]) => {
            addActivity(method, path);
        });

        // Mise à jour périodique (toutes les 3 secondes)
        setInterval(updateDashboard, 3000);

        // Activité aléatoire (toutes les 4-8 secondes)
        setInterval(() => {
            if (Math.random() < 0.5) {
                generateRandomActivity();
            }
        }, randomBetween(4000, 8000));

        // Événements
        if (elements.refreshBtn) {
            elements.refreshBtn.addEventListener('click', handleRefresh);
        }

        if (elements.clearActivityBtn) {
            elements.clearActivityBtn.addEventListener('click', clearActivity);
        }

        console.log('[LITH] Dashboard initialized successfully');
        console.log(`[LITH] ${new Date().toISOString()}`);
    }

    // Démarrer quand le DOM est prêt
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }

})();