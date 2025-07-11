#include "sistema.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <queue>
#include <algorithm>
#include <corecrt_math_defines.h>

constexpr double R = 6371000.0;
constexpr double DEG_TO_RAD = M_PI / 180.0;
const std::vector<RutaGeografica>& Sistema::getRutas() const {
    return rutas;
}

double Sistema::distanciaCoord(const Punto& a, const Punto& b) const {
    double dLat = (b.lat - a.lat) * DEG_TO_RAD;
    double dLon = (b.lon - a.lon) * DEG_TO_RAD;
    double lat1 = a.lat * DEG_TO_RAD;
    double lat2 = b.lat * DEG_TO_RAD;

    double aVal = sin(dLat / 2) * sin(dLat / 2) +
                  cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);
    return R * 2 * atan2(sqrt(aVal), sqrt(1 - aVal));
}

std::pair<int, int> Sistema::getCellKey(double lat, double lon, double cellSize) const {
    int x = static_cast<int>(floor(lon / cellSize));
    int y = static_cast<int>(floor(lat / cellSize));
    return {x, y};
}
void Sistema::cargarTodoSiEsNecesario() {
    if (rutasCargadas) return;

    cargarShapesCSV("datos/shapes.csv");
    std::cout << "[DEBUG] Shapes cargados: " << shapesPorShapeId.size() << "\n";

    cargarTripsCSV("datos/trips.csv");
    std::cout << "[DEBUG] Trips cargados: " << shapeToRoute.size() << "\n";

    cargarRoutesCSV("datos/routes.csv");
    std::cout << "[DEBUG] Routes cargadas: " << rutasMetadata.size() << "\n";

    construirRutas();
    std::cout << "[DEBUG] Rutas construidas: " << rutas.size() << "\n";

    rutasCargadas = true;
    std::cout << "[INFO] CSV cargados dinámicamente.\n";
}



void Sistema::cargarShapesCSV(const std::string& archivoShapes) {
    std::ifstream file(archivoShapes);
    std::string linea;

    // Ignorar encabezado
    std::getline(file, linea);

    while (std::getline(file, linea)) {
        std::stringstream ss(linea);
        std::string shape_id, lat, lon, seq, dist;
        std::getline(ss, shape_id, ',');
        std::getline(ss, lat, ',');
        std::getline(ss, lon, ',');
        std::getline(ss, seq, ',');
        std::getline(ss, dist, ',');

        try {
            Punto p{ std::stod(lat), std::stod(lon) };
            shapesPorShapeId[shape_id].push_back(p);
        } catch (...) {
            std::cerr << "[ERROR] Línea inválida en shapes.csv: " << linea << "\n";
        }
    }
}


void Sistema::cargarTripsCSV(const std::string& archivoTrips) {
    std::ifstream file(archivoTrips);
    std::string linea;

    // Ignorar encabezado
    std::getline(file, linea);

    while (std::getline(file, linea)) {
        std::stringstream ss(linea);
        std::string route_id, trip_id, headsign, short_name, dir_id, shape_id;
        std::getline(ss, route_id, ',');
        std::getline(ss, trip_id, ',');
        std::getline(ss, headsign, ',');
        std::getline(ss, short_name, ',');
        std::getline(ss, dir_id, ',');
        std::getline(ss, shape_id, ',');

        if (!shape_id.empty() && !route_id.empty()) {
            shapeToRoute[shape_id] = route_id;
        }
    }
}

void Sistema::cargarRoutesCSV(const std::string& archivoRoutes) {
    std::ifstream file(archivoRoutes);
    std::string linea;

    // Ignorar encabezado
    std::getline(file, linea);

    while (std::getline(file, linea)) {
        std::stringstream ss(linea);
        std::string route_id, short_name, long_name, desc;
        std::getline(ss, route_id, ',');
        std::getline(ss, short_name, ',');
        std::getline(ss, long_name, ',');
        std::getline(ss, desc, ',');

        if (!route_id.empty()) {
            rutasMetadata[route_id] = {short_name, long_name, desc};
        }
    }
}

void Sistema::construirRutas() {
    rutas.clear();
    std::unordered_map<std::string, std::vector<Punto>> rutasPorId;

    // 🔧 Asociar shape_id → route_id y juntar los puntos
    for (const auto& [shape_id, puntos] : shapesPorShapeId) {
        auto it = shapeToRoute.find(shape_id);
        if (it != shapeToRoute.end()) {
            const std::string& route_id = it->second;
            rutasPorId[route_id].insert(rutasPorId[route_id].end(), puntos.begin(), puntos.end());
        }
    }

    // ✅ Construir las rutas geográficas con metadata (si existe)
    for (const auto& [route_id, puntos] : rutasPorId) {
        RutaGeografica ruta;
        ruta.id = route_id;
        ruta.puntos = puntos;

        auto it = rutasMetadata.find(route_id);
        if (it != rutasMetadata.end()) {
            std::tie(ruta.nombreCorto, ruta.nombreLargo, ruta.descripcion) = it->second;
        }

        rutas.push_back(ruta);
    }
}

void Sistema::construirGrafoDeTransbordos(double maxDistanciaMetros) {
    grafo.clear();
    double cellSize = maxDistanciaMetros / 111000.0;
    std::unordered_map<std::string, std::vector<std::tuple<std::string, double, double>>> grid;

    // Indexar puntos
    for (const auto& r : rutas) {
        grafo[r.id]; // crear entrada vacía
        for (const auto& p : r.puntos) {
            auto key = getCellKey(p.lat, p.lon, cellSize);
            std::string kstr = std::to_string(key.first) + "," + std::to_string(key.second);
            grid[kstr].emplace_back(r.id, p.lat, p.lon);
        }
    }

    // Buscar conexiones por cercanía
    int offset[] = {-1, 0, 1};
    for (const auto& r : rutas) {
        std::unordered_set<std::string> visitados;
        for (const auto& p : r.puntos) {
            auto [cx, cy] = getCellKey(p.lat, p.lon, cellSize);
            for (int dx : offset) {
                for (int dy : offset) {
                    std::string vecinoKey = std::to_string(cx + dx) + "," + std::to_string(cy + dy);
                    if (!grid.count(vecinoKey)) continue;

                    for (const auto& [otraRutaId, lat, lon] : grid[vecinoKey]) {
                        if (otraRutaId == r.id || visitados.count(otraRutaId)) continue;
                        double dist = distanciaCoord(p, {lat, lon});
                        if (dist <= maxDistanciaMetros) {
                            grafo[r.id].insert(otraRutaId);
                            grafo[otraRutaId].insert(r.id);
                            visitados.insert(otraRutaId);
                        }
                    }
                }
            }
        }
    }
}
std::vector<std::string> Sistema::rutasCercanas(const Punto& punto, double maxDistancia) const {
    std::vector<std::string> rutasCerca;
    double cellSize = maxDistancia / 111000.0;

    int cx = static_cast<int>(floor(punto.lon / cellSize));
    int cy = static_cast<int>(floor(punto.lat / cellSize));
    int offset[] = {-1, 0, 1};

    std::unordered_set<std::string> resultado;

    for (const auto& ruta : rutas) {
        for (const auto& p : ruta.puntos) {
            int px = static_cast<int>(floor(p.lon / cellSize));
            int py = static_cast<int>(floor(p.lat / cellSize));

            bool cerca = false;
            for (int dx : offset) {
                for (int dy : offset) {
                    if (px == cx + dx && py == cy + dy) {
                        if (distanciaCoord(p, punto) <= maxDistancia) {
                            resultado.insert(ruta.id);
                            cerca = true;
                            break;
                        }
                    }
                }
                if (cerca) break;
            }
        }
    }

    return std::vector<std::string>(resultado.begin(), resultado.end());
}
std::vector<std::string> Sistema::buscarRutaAvanzada(
    const Punto& origen,
    const Punto& destino,
    double maxDistanciaCaminata,
    double maxDistanciaTransbordo,
    int maxTransbordos
) {
    construirGrafoDeTransbordos(maxDistanciaTransbordo);

    auto rutasOrigen = rutasCercanas(origen, maxDistanciaCaminata);
    auto rutasDestino = rutasCercanas(destino, maxDistanciaCaminata);

    if (rutasOrigen.empty() || rutasDestino.empty()) {
        std::cerr << "No hay rutas cercanas al origen o destino.\n";
        return {};
    }

    std::unordered_set<std::string> destinos(rutasDestino.begin(), rutasDestino.end());
    std::unordered_set<std::string> visitados;
    std::queue<std::pair<std::string, std::vector<std::string>>> cola;

    for (const auto& r : rutasOrigen) {
        cola.push({r, {r}});
        visitados.insert(r);
    }

    while (!cola.empty()) {
        auto [actual, camino] = cola.front();
        cola.pop();

        if (destinos.count(actual)) {
            return camino;
        }

        if (static_cast<int>(camino.size()) - 1 > maxTransbordos) continue;

        for (const auto& vecino : grafo.at(actual)) {
            if (!visitados.count(vecino)) {
                auto nuevoCamino = camino;
                nuevoCamino.push_back(vecino);
                cola.push({vecino, nuevoCamino});
                visitados.insert(vecino);
            }
        }
    }

    std::cerr << "No se encontró ruta con los parámetros dados.\n";
    return {};
}
