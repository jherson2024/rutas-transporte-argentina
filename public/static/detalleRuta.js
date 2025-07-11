function mostrarDetalleRuta(contenidoHTML) {
    const overlay = document.getElementById('overlay-detalle');
    const detalle = document.getElementById('detalle-ruta');

    detalle.innerHTML = contenidoHTML;
    overlay.classList.remove('oculto');
    overlay.classList.add('activo');
    document.body.classList.add('modal-abierto');
}

document.getElementById('cerrar-detalle').addEventListener('click', () => {
    const overlay = document.getElementById('overlay-detalle');
    overlay.classList.remove('activo');
    overlay.classList.add('oculto');
    document.body.classList.remove('modal-abierto');
});