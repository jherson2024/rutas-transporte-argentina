function cargarRutasDesdeServidor() {
    fetch('/api', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ operacion: 'listar_rutas' })
    })
    .then(res => {
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        return res.json();
    })
    .then(rutas => {
        if (!Array.isArray(rutas)) {
            console.error("❌ Respuesta inesperada: 'rutas' no es un array", rutas);
            return;
        }

        console.log("✅ Rutas cargadas:", rutas);

        // Limpiar capas anteriores si es necesario
        drawnItems.clearLayers();

        rutas.forEach(ruta => {
            try {
                if (!Array.isArray(ruta.puntos) || ruta.puntos.length === 0) {
                    console.warn(`⚠️ Ruta ${ruta.id} sin puntos válidos.`);
                    return;
                }

                const puntosOrdenados = ruta.puntos
                    .filter(p => typeof p.lat === 'number' && typeof p.lng === 'number')
                    .sort((a, b) => a.orden - b.orden);

                if (puntosOrdenados.length === 0) return;

                const latlngs = puntosOrdenados.map(p => [p.lat, p.lng]);
                const color = ruta.color || getColorForRuta(ruta.id);

                const polyline = L.polyline(latlngs, { color }).addTo(drawnItems);
                polyline._rutaId = ruta.id;
                polyline.bindPopup(`
                    <strong>${ruta.nombre || "Ruta " + ruta.id}</strong><br>
                    <em>${ruta.descripcion || "Sin descripción"}</em>
                `);

                // Agregar a la lista
                const lista = document.getElementById('lista-rutas');
                if (lista) {
                    const li = document.createElement('li');
                    li.innerHTML = `
                        <span style="display:inline-block;width:12px;height:12px;background:${color};margin-right:6px;border-radius:2px;"></span>
                        <strong>${ruta.nombre || "Ruta " + ruta.id}</strong>
                        <button onclick="verRuta(${JSON.stringify(ruta.id)})">👁️</button>
                        <button onclick="verModalRuta(${JSON.stringify(ruta.id)})">ℹ️</button>
                        <button onclick="editarRuta(${JSON.stringify(ruta.id)})">🖉</button>
                        <button onclick="eliminarRuta(${JSON.stringify(ruta.id)})">🗑️</button>
                    `;
                    lista.appendChild(li);
                }

            } catch (err) {
                console.error(`❌ Error en ruta ID ${ruta.id}:`, err);
            }
        });

        // Ajustar el mapa
        const capas = drawnItems.getLayers();
        if (capas.length > 0) {
            map.fitBounds(drawnItems.getBounds());
            console.log("📍 Mapa ajustado a las rutas cargadas.");
        } else {
            console.warn("🛑 No se dibujaron capas en el mapa.");
        }

        window.rutasCargadas = rutas;
    })
    .catch(err => {
        console.error("❌ Error al cargar rutas desde el servidor:", err);
        alert("Error al cargar las rutas. Revisa la consola.");
    });
}
