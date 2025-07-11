#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct Punto {
    double lat;
    double lon;
};

struct RutaGeografica {
    std::string id;                     // route_id
    std::string nombreCorto;           // route_short_name
    std::string nombreLargo;           // route_long_name
    std::string descripcion;           // route_desc
    std::vector<Punto> puntos;         // puntos GPS
};

class Sistema {
public:
    // Cargar datos desde CSVs
    void cargarShapesCSV(const std::string& archivoShapes);
    void cargarTripsCSV(const std::string& archivoTrips);
    // Construcción de rutas por shape_id -> route_id
    void construirRutas();
    void cargarRoutesCSV(const std::string& archivoRoutes);
    const std::vector<RutaGeografica>& getRutas() const;
    void cargarTodoSiEsNecesario();
    // Construcción del grafo de conexiones entre rutas (por proximidad)
    void construirGrafoDeTransbordos(double maxDistanciaMetros = 250.0);

    // Buscar rutas cercanas a un punto (para origen/destino físico)
    std::vector<std::string> rutasCercanas(const Punto& punto, double maxDistancia) const;

    // Buscar ruta entre coordenadas geográficas con parámetros avanzados
    std::vector<std::string> buscarRutaAvanzada(
        const Punto& origen,
        const Punto& destino,
        double maxDistanciaCaminata,
        double maxDistanciaTransbordo,
        int maxTransbordos
    );
    std::unordered_map<std::string, std::tuple<std::string, std::string, std::string>> rutasMetadata;

private:
    // Datos crudos cargados
    std::unordered_map<std::string, std::vector<Punto>> shapesPorShapeId;
    std::unordered_map<std::string, std::string> shapeToRoute;
    bool rutasCargadas = false;
    // Rutas construidas por route_id
    std::vector<RutaGeografica> rutas;

    // Grafo de conexiones entre rutas
    std::unordered_map<std::string, std::unordered_set<std::string>> grafo;

    // Utilidades internas
    double distanciaCoord(const Punto& a, const Punto& b) const;
    std::pair<int, int> getCellKey(double lat, double lon, double cellSize) const;
};
