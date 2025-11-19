$(document).ready(function() {
    let ultimoTimestamp = 0;
    const noBiodegradables = ["No biodegradable", "plastico", "metal", "vidrio"];

    function actualizarInterfaz(datos) {
        if (datos.imagen_path) {
            $('#imagen-clasificada').css('opacity', '0.5');
            $('#imagen-clasificada').attr('src', '/static/' + datos.imagen_path);
            setTimeout(() => {
                $('#imagen-clasificada').css('opacity', '1');
            }, 300);
        }
        $('#clase-modelo').text(datos.clase || 'Esperando...');
        
        let prob = datos.probabilidad ? (datos.probabilidad * 100).toFixed(1) : "0.0";
        $('#probabilidad').text(prob + '%');
        $('#clase-modelo').removeClass('biodegradable no-biodegradable reintentar');
        if (datos.clase === "Objeto no identificado. Reintentar") {
            $('#clase-modelo').addClass('reintentar');
        } 
        else if (noBiodegradables.includes(datos.clase)) {
            $('#clase-modelo').addClass('no-biodegradable');
        } 
        else {
            $('#clase-modelo').addClass('biodegradable');
        }
    }

    function buscarActualizaciones() {
        $.ajax({
            url: '/verificar_actualizacion',
            type: 'GET',
            data: { timestamp: ultimoTimestamp },
            success: function(response) {
                if (response.actualizado) {
                    ultimoTimestamp = response.timestamp;
                    actualizarInterfaz(response.datos);
                }
                buscarActualizaciones();
            },
            error: function(error) {
                console.error(error)
                setTimeout(buscarActualizaciones, 5000);
            }
        });
    }

    buscarActualizaciones();
});