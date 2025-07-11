let grafoRutas = null;
let ultimaDistanciaTransbordo = null;
let marcadorOrigen = null;
let marcadorDestino = null;
let modoSeleccion = null;
function seleccionarCoordenada(tipo) {
    modoSeleccion = tipo;
    alert(`Haz clic en el mapa para seleccionar el ${tipo}`);
}
function buscarRuta() {
    
    const origenStr = document.getElementById("origen").value.trim();
    const destinoStr = document.getElementById("destino").value.trim();
    const distCaminar = parseFloat(document.getElementById("distanciaMaxCaminata").value);
    const distTransbordo = parseFloat(document.getElementById("distanciaMaxTransbordo").value);
    const maxTransbordos = parseInt(document.getElementById("maxTransbordos").value);

    if (!origenStr || !destinoStr) {
        alert("Por favor, completá el origen y el destino.");
        return;
    }

    const [lat_origen, lng_origen] = origenStr.split(",").map(Number);
    const [lat_destino, lng_destino] = destinoStr.split(",").map(Number);

    if (
        isNaN(lat_origen) || isNaN(lng_origen) ||
        isNaN(lat_destino) || isNaN(lng_destino)
    ) {
        alert("Las coordenadas ingresadas no son válidas.");
        return;
    }

    const body = {
        operacion: "buscar_ruta",
        origen: { lat: lat_origen, lon: lng_origen },
        destino: { lat: lat_destino, lon: lng_destino },
        distanciaCaminata: distCaminar,
        distanciaTransbordo: distTransbordo,
        maxTransbordos: maxTransbordos
    };
    
    fetch("/api", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
    })
        .then(res => {
            if (!res.ok) throw new Error("Error al buscar ruta");
            return res.json();
        })
        .then(data => {
            const resultado = document.getElementById("resultado-busqueda");
            resultado.innerHTML = ""; // Limpiar resultado anterior

            if (!Array.isArray(data) || data.length === 0) {
                resultado.innerHTML = "<p>❌ No se encontraron rutas disponibles.</p>";
                return;
            }

            data.forEach(ruta => {
                const color = getColorForRuta(parseInt(ruta.id));
                const puntos = ruta.puntos.map(p => [p.lat, p.lng]);

                // Estilo destacado (guionado, grueso)
                const polyline = L.polyline(puntos, {
                    color: color,
                    weight: 6,       // línea más gruesa
                    opacity: 1,      // opacidad total
                    lineJoin: 'round'
                }).addTo(drawnItems);
                // Mostrar en panel con color
                resultado.innerHTML += `
                    <div style="display: flex; align-items: center; gap: 8px; margin-bottom: 6px;">
                        <span style="display: inline-block; width: 16px; height: 16px; background-color: ${color}; border-radius: 3px;"></span>
                        <div>
                            <strong>${ruta.nombre}</strong><br>
                            <em>${ruta.descripcion || "Sin descripción"}</em>
                        </div>
                    </div><hr>
                `;
            });

        })
        .catch(err => {
            console.error("❌ Error al buscar ruta:", err);
            alert("Hubo un error al buscar la ruta.");
        });
}
