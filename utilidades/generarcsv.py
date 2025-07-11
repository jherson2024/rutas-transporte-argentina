import pymysql
import csv

# Configura tu conexión MySQL aquí
conn = pymysql.connect(
    host='localhost',
    user='root',
    password='1234',
    db='gtfs_data',
    charset='utf8mb4'
)

def export_to_csv(query, filename):
    with conn.cursor() as cursor:
        cursor.execute(query)
        rows = cursor.fetchall()
        headers = [desc[0] for desc in cursor.description]

        with open(filename, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(headers)
            writer.writerows(rows)

# 🔹 1. Obtener 200 route_id
with conn.cursor() as cursor:
    cursor.execute("SELECT route_id FROM routes LIMIT 200")
    route_ids = [row[0] for row in cursor.fetchall()]

# 🔹 2. Convertir a cadena SQL segura
route_id_list = ",".join(f"'{r}'" for r in route_ids)

# 🔹 3. Exportar rutas
export_to_csv(f"""
    SELECT * FROM routes
    WHERE route_id IN ({route_id_list})
""", "routes.csv")

# 🔹 4. Exportar 1 trip por ruta (el de menor trip_id)
export_to_csv(f"""
    SELECT t.* FROM trips t
    JOIN (
        SELECT route_id, MIN(trip_id) AS trip_id
        FROM trips
        WHERE route_id IN ({route_id_list})
        GROUP BY route_id
    ) AS mintrips
    ON t.route_id = mintrips.route_id AND t.trip_id = mintrips.trip_id
""", "trips.csv")

# 🔹 5. Exportar shapes asociados a esos trips
export_to_csv(f"""
    SELECT shapes.* FROM shapes
    JOIN (
        SELECT DISTINCT shape_id FROM trips
        WHERE route_id IN ({route_id_list})
        GROUP BY shape_id
    ) AS s
    ON shapes.shape_id = s.shape_id
""", "shapes.csv")

conn.close()
print("✅ Archivos generados correctamente: routes.csv, trips.csv, shapes.csv")
