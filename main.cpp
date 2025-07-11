#define _CRT_SECURE_NO_WARNINGS
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/http_msg.h>
#include <cpprest/details/basic_types.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <string>
#include "sistema.h"
using namespace utility::conversions;
using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
//datos en memoria
Sistema sistema;  
//utilidades
std::string to_ascii_string(const utility::string_t& input) {
    std::string result;
    for (wchar_t wc : input) result += static_cast<char>(wc);
    return result;
}
void enviar_json(http_request request, const json::value& body) {
    http_response response(status_codes::OK);
    response.headers().add(U("Content-Type"), U("application/json"));
    response.set_body(body);
    request.reply(response);
}
void handle_get(http_request request) {
    auto path = uri::decode(request.relative_uri().path());
    std::string archivo = path == U("/") ? "index.html" : utility::conversions::to_utf8string(path.substr(1));
    std::string ruta = "public/" + archivo;
    std::ifstream file(ruta, std::ios::binary);
    if (!file) {
        request.reply(status_codes::NotFound, U("Archivo no encontrado"));
        return;
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string contenido = buffer.str();
    std::string content_type = "application/octet-stream";
    if (ruta.ends_with(".html"))       content_type="text/html";
    else if (ruta.ends_with(".css"))   content_type="text/css";
    else if (ruta.ends_with(".js"))    content_type="application/javascript";
    else if (ruta.ends_with(".json"))  content_type="application/json";
    else if (ruta.ends_with(".svg"))   content_type="image/svg+xml";
    else if (ruta.ends_with(".png"))   content_type="image/png";
    else if (ruta.ends_with(".jpg") || ruta.ends_with(".jpeg")) content_type = "image/jpeg";
    http_response response(status_codes::OK);
    response.headers().add(U("Content-Type"), utility::conversions::to_string_t(content_type));
    response.set_body(contenido);
    request.reply(response);
}
void handle_post(http_request request) {
    
    request.extract_json().then([request](json::value body) {
        try {
            std::string operacion = to_ascii_string(body[U("operacion")].as_string());
            std::cout << "[API] Operación recibida: " << operacion << std::endl;

            if (operacion == "buscar_ruta") {
                double lat_origen = body[U("origen")][U("lat")].as_double();
                double lon_origen = body[U("origen")][U("lon")].as_double();
                double lat_destino = body[U("destino")][U("lat")].as_double();
                double lon_destino = body[U("destino")][U("lon")].as_double();
                double distCaminar = body[U("distanciaCaminata")].as_double();
                double distTransbordo = body[U("distanciaTransbordo")].as_double();
                int maxTransbordos = body[U("maxTransbordos")].as_integer();

                Punto origen{lat_origen, lon_origen};
                Punto destino{lat_destino, lon_destino};

                auto rutaIds = sistema.buscarRutaAvanzada(origen, destino, distCaminar, distTransbordo, maxTransbordos);

                // Buscar objetos RutaGeografica con esos IDs
                json::value resultado = json::value::array();
                int i = 0;
                for (const auto& id : rutaIds) {
                    for (const auto& r : sistema.getRutas()) {
                        if (r.id == id) {
                            json::value obj;
                            obj[U("id")] = json::value::string(to_string_t(r.id));
                            obj[U("nombre")] = json::value::string(to_string_t(r.nombreCorto));
                            obj[U("descripcion")] = json::value::string(to_string_t(r.descripcion));

                            json::value puntos = json::value::array();
                            int j = 0;
                            for (const auto& p : r.puntos) {
                                json::value punto;
                                punto[U("lat")] = p.lat;
                                punto[U("lng")] = p.lon;
                                punto[U("orden")] = j++;
                                puntos[j - 1] = punto;
                            }
                            obj[U("puntos")] = puntos;

                            resultado[i++] = obj;
                            break;
                        }
                    }
                }

                enviar_json(request, resultado);
            }

            else if (operacion == "listar_rutas") {
                sistema.cargarTodoSiEsNecesario();
                const auto& rutas = sistema.getRutas();
                std::cout << "[INFO] Se encontraron " << rutas.size() << " rutas." << std::endl;

                json::value resultado = json::value::array();
                int i = 0;
                for (const auto& r : rutas) {
                    std::cout << " - Ruta " << r.id << " con " << r.puntos.size() << " puntos." << std::endl;

                    json::value obj;
                    obj[U("id")] = json::value::string(to_string_t(r.id));
                    obj[U("nombre")] = json::value::string(to_string_t(r.nombreCorto));
                    obj[U("descripcion")] = json::value::string(to_string_t(r.descripcion));

                    json::value puntos = json::value::array();
                    int j = 0;
                    for (const auto& p : r.puntos) {
                        json::value punto;
                        punto[U("lat")] = p.lat;
                        punto[U("lng")] = p.lon;
                        punto[U("orden")] = j++;
                        puntos[j - 1] = punto;
                    }
                    obj[U("puntos")] = puntos;

                    resultado[i++] = obj;
                }

                std::cout << "[INFO] Enviando JSON con " << i << " rutas al cliente." << std::endl;
                enviar_json(request, resultado);
            }

            else if (operacion == "detalle_ruta") {
                std::string id = to_ascii_string(body[U("id")].as_string());

                const auto& rutas = sistema.getRutas();
                auto it = std::find_if(rutas.begin(), rutas.end(), [&](const RutaGeografica& r) {
                    return r.id == id;
                });

                if (it == rutas.end()) {
                    request.reply(status_codes::NotFound, U("Ruta no encontrada"));
                    return;
                }

                const RutaGeografica& r = *it;

                json::value obj;
                obj[U("id")] = json::value::string(to_string_t(r.id));
                obj[U("nombre")] = json::value::string(to_string_t(r.nombreCorto));
                obj[U("descripcion")] = json::value::string(to_string_t(r.descripcion));
                enviar_json(request, obj);
            }


            else {
                request.reply(status_codes::BadRequest, U("Operación desconocida"));
            }

        } catch (const std::exception& e) {
            std::cerr << "Error en POST: " << e.what() << "\n";
            request.reply(status_codes::BadRequest, U("Error en los datos del POST"));
        }
    });
}
// ========== MAIN ==========
int main() {
    try {

        http_listener listener(U("http://localhost:5000"));
        listener.support(methods::GET, handle_get);
        listener.support(methods::POST, handle_post);

        listener.open().wait();
        std::cout << "Servidor activo en http://localhost:5000\n";
        std::cout << "Presiona ENTER para cerrar...\n";

        std::string dummy;
        std::getline(std::cin, dummy);
        listener.close().wait();
    } catch (const std::exception& e) {
        std::cerr << "Error al iniciar el servidor: " << e.what() << std::endl;
    }

    return 0;
}
