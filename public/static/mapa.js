
const map = L.map('map').setView([-34.6037, -58.3816], 13);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    maxZoom: 19,
}).addTo(map);
// Crear grupo de capas dibujadas
window.drawnItems = new L.FeatureGroup();
map.addLayer(drawnItems);
// Control de dibujo
const drawControl = new L.Control.Draw({
    draw: {
        polyline: { shapeOptions: { color: getRandomColor() } },
        polygon: false,
        rectangle: false,
        circle: false,
        marker: false,
        circlemarker: false
    },
    edit: { featureGroup: drawnItems }
});
map.addControl(drawControl);
// Funciones auxiliares
function getColorForRuta(id) {
    const colors = [
        '#e6194b', '#3cb44b', '#ffe119', '#4363d8',
        '#f58231', '#911eb4', '#46f0f0', '#f032e6',
        '#bcf60c', '#fabebe', '#008080', '#e6beff'
    ];
    return colors[id % colors.length];
}

function getRandomColor() {
    const colors = [
        '#e6194b', '#3cb44b', '#ffe119', '#4363d8',
        '#f58231', '#911eb4', '#46f0f0', '#f032e6',
        '#bcf60c', '#fabebe', '#008080', '#e6beff'
    ];
    return colors[Math.floor(Math.random() * colors.length)];
}
function verRuta(id) {
   fetch('/', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ operacion: 'detalle_ruta', id })
})

        .then(res => {
            if (!res.ok) throw new Error("Ruta no encontrada");
            return res.json();
        })
        .then(ruta => {
            if (!Array.isArray(ruta.puntos) || ruta.puntos.length === 0) {
                alert("Ruta no encontrada o sin puntos.");
                return;
            }

            const coords = ruta.puntos
                .filter(p => typeof p.lat === 'number' && typeof p.lng === 'number')
                .sort((a, b) => a.orden - b.orden);

            if (coords.length === 0) {
                alert("Esta ruta no tiene coordenadas válidas.");
                return;
            }

            // Mostrar detalles en el modal
            const detalle = document.getElementById('detalle-ruta');
            detalle.innerHTML = `
                <h3>${ruta.nombre}</h3>
                <p><strong>Descripción:</strong> ${ruta.descripcion || 'Sin descripción'}</p>
                <p><strong>Trip:</strong> ${ruta.trip_id || 'N/A'}</p>
                <p><strong>Dirección:</strong> ${ruta.direction_id ?? 'N/A'}</p>
            `;

            // Dibujar la ruta en el mapa
            const latlngs = coords.map(c => [c.lat, c.lng]);
            const polyline = L.polyline(latlngs, {
                color: getColorForRuta(ruta.id),
                weight: 4,
                opacity: 0.9
            }).addTo(map);

            map.fitBounds(polyline.getBounds());

            setTimeout(() => {
                map.removeLayer(polyline);
            }, 8000);
        })
        .catch(err => {
            console.error("Error al ver ruta:", err);
            alert("Error cargando datos de la ruta.");
        });
}
function verModalRuta(id) {
    fetch("/ruta", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
            operacion: "detalle_ruta",
            id: id
        })
    })
        .then(res => {
            if (!res.ok) throw new Error("Ruta no encontrada");
            return res.json();
        })
        .then(ruta => {
            if (!ruta) {
                alert("Ruta no encontrada.");
                return;
            }

            const contenidoHTML = `
                <h3>${ruta.nombre || "Ruta " + ruta.id}</h3>
                <p><strong>ID:</strong> ${ruta.id}</p>
                <p><strong>Descripción:</strong> ${ruta.descripcion || 'Sin descripción'}</p>
            `;

            mostrarDetalleRuta(contenidoHTML);
        })
        .catch(err => {
            console.error("Error al cargar detalles:", err);
            alert("No se pudo cargar el detalle de la ruta.");
        });
}


map.on('click', function(e) {
    if (!modoSeleccion) return;

    const { lat, lng } = e.latlng;
    const valor = `${lat.toFixed(6)},${lng.toFixed(6)}`;
    document.getElementById(modoSeleccion).value = valor;

    // Crear o actualizar el marcador correspondiente
    if (modoSeleccion === 'origen') {
        if (marcadorOrigen) {
            marcadorOrigen.setLatLng(e.latlng);
        } else {
            marcadorOrigen = L.marker(e.latlng, { draggable: true }).addTo(map);
            marcadorOrigen.on('dragend', function (ev) {
                const pos = ev.target.getLatLng();
                document.getElementById('origen').value = `${pos.lat.toFixed(6)},${pos.lng.toFixed(6)}`;
            });
        }
    } else if (modoSeleccion === 'destino') {
        if (marcadorDestino) {
            marcadorDestino.setLatLng(e.latlng);
        } else {
            marcadorDestino = L.marker(e.latlng, { draggable: true, icon: L.icon({
                iconUrl: 'https://cdn-icons-png.flaticon.com/512/684/684908.png',
                iconSize: [25, 41],
                iconAnchor: [12, 41],
                popupAnchor: [1, -34],
                shadowSize: [41, 41]
            }) }).addTo(map);
            marcadorDestino.on('dragend', function (ev) {
                const pos = ev.target.getLatLng();
                document.getElementById('destino').value = `${pos.lat.toFixed(6)},${pos.lng.toFixed(6)}`;
            });
        }
    }

    modoSeleccion = null;
});

