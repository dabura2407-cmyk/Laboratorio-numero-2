/* ============================================================
   LABORATORIO VIRTUAL DE ELECTRONICA - PROTOBOARD SIMPLE (v2)
   ============================================================
   Proyecto educativo mejorado:
   - Entrada de usuario robusta con fgets
   - Visualización del circuito armado
   - Eliminación de resistencias
   - Cálculo separado de corriente
   - Comparación de colores sin distinción de mayúsculas
   - Mensajes más didácticos
   - Validaciones extra para mayor seguridad
   ============================================================ */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_RESISTENCIAS 5
#define VOLTAJE_BATERIA 9.0
#define CORRIENTE_MIN 0.005      /* 5 mA */
#define CORRIENTE_MAX 0.020      /* 20 mA */

typedef struct {
    double resistencias[MAX_RESISTENCIAS];
    int numResistencias;
    int hayLed;
    char colorLed[15];
    double voltajeLed;           /* Vf según color */
} Circuito;

/* ---------- Funciones auxiliares de entrada ---------- */

/* Limpia el buffer de entrada hasta el siguiente salto de línea */
void limpiarBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Lee una línea y extrae un double. Devuelve 1 si éxito, 0 si error. */
int leerDouble(double *valor) {
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;
    /* Reemplazar coma por punto si existe (para localización) */
    char *p = buffer;
    while (*p) {
        if (*p == ',') *p = '.';
        p++;
    }
    if (sscanf(buffer, "%lf", valor) == 1)
        return 1;
    return 0;
}

/* Lee una línea y extrae una cadena (sin espacios). */
int leerString(char *dest, int tam) {
    char buffer[100];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;
    /* Eliminar el salto de línea final */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n')
        buffer[len-1] = '\0';
    /* Copiar de forma segura */
    strncpy(dest, buffer, tam - 1);
    dest[tam - 1] = '\0';
    return 1;
}

/* Convierte una cadena a minúsculas (para comparar colores) */
void aMinusculas(char *s) {
    while (*s) {
        *s = tolower(*s);
        s++;
    }
}

/* ---------- Asistente IA: sugerencia de resistencia ---------- */
/* Todo lo de esta seccion usa arreglos de tamano fijo (stack),
   NUNCA memoria dinamica. Nada de malloc/calloc/realloc/free. */

#define NUM_VALORES_COMERCIALES 12

/* Valores comerciales tipicos (serie E12), a proposito sin ordenar,
   para demostrar el ordenamiento antes de buscar. */
double valoresComerciales[NUM_VALORES_COMERCIALES] = {
    330, 100, 1000, 220, 680, 120, 470, 150, 820, 270, 560, 390
};

/* Ordena un arreglo de doubles de menor a mayor (insertion sort).
   Es O(n^2), pero aqui n es pequeno (12), asi que es instantaneo.
   No usa memoria adicional: ordena en el mismo arreglo (in-place). */
void ordenarInsercion(double arr[], int n) {
    for (int i = 1; i < n; i++) {
        double clave = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > clave) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = clave;
    }
}

/* Busqueda binaria tipo "lower_bound": devuelve el indice del primer
   elemento de arr (ya ordenado) que es MAYOR O IGUAL a valor.
   Si no existe ninguno, devuelve n. Trabaja sobre el arreglo que le
   pasan, sin reservar memoria nueva. */
int lowerBound(double arr[], int n, double valor) {
    int inicio = 0, fin = n;
    while (inicio < fin) {
        int medio = inicio + (fin - inicio) / 2;
        if (arr[medio] < valor)
            inicio = medio + 1;
        else
            fin = medio;
    }
    return inicio;
}

/* Asistente "IA" muy simple basado en reglas + busqueda binaria:
   calcula la resistencia minima necesaria para no quemar el LED y
   busca con lower_bound el valor comercial mas pequeno que la cumple. */
void asistenteSugerirResistencia(Circuito *c) {
    if (!c->hayLed) {
        printf("\n[Asistente IA] Agrega primero un LED para poder sugerirte una resistencia.\n");
        return;
    }

    double vDisponible = VOLTAJE_BATERIA - c->voltajeLed;
    if (vDisponible <= 0) {
        printf("\n[Asistente IA] El LED %s no puede funcionar con esta bateria (Vf demasiado alto).\n",
               c->colorLed);
        return;
    }

    /* Resistencia minima para que la corriente no supere el maximo seguro */
    double rMinima = vDisponible / CORRIENTE_MAX;

    int idx = lowerBound(valoresComerciales, NUM_VALORES_COMERCIALES, rMinima);

    printf("\n[Asistente IA]\n");
    printf("Para el LED %s necesitas al menos %.1f Ohm para no quemarlo.\n", c->colorLed, rMinima);

    if (idx >= NUM_VALORES_COMERCIALES) {
        printf("Ningun valor comercial de la lista es suficientemente grande.\n");
        printf("Te recomendamos usar %.1f Ohm o mas.\n", rMinima);
    } else {
        double sugerida = valoresComerciales[idx];
        double corrienteConSugerida = vDisponible / sugerida;
        printf("Resistencia comercial recomendada: %.1f Ohm\n", sugerida);
        printf("Con ese valor la corriente seria de %.2f mA (dentro del rango seguro).\n",
               corrienteConSugerida * 1000);
    }
}

/* ---------- Funciones del circuito ---------- */

/* Devuelve el voltaje directo según el color (en minúsculas) */
double voltajeSegunColor(const char *color) {
    if (strcmp(color, "rojo") == 0)    return 1.8;
    if (strcmp(color, "verde") == 0)   return 2.1;
    if (strcmp(color, "amarillo") == 0)return 2.0;
    if (strcmp(color, "azul") == 0)    return 3.0;
    return -1;
}

/* Calcula la corriente total del circuito (Ley de Ohm) */
double calcularCorriente(Circuito *c) {
    double rTotal = 0;
    for (int i = 0; i < c->numResistencias; i++)
        rTotal += c->resistencias[i];
    if (rTotal <= 0) return -1;  /* corto circuito */
    double vDisponible = VOLTAJE_BATERIA - c->voltajeLed;
    if (vDisponible <= 0) return 0; /* voltaje insuficiente */
    return vDisponible / rTotal;
}

/* Muestra el estado actual del circuito */
void mostrarCircuito(Circuito *c) {
    printf("\n--- CIRCUITO ACTUAL ---\n");
    printf("Batería: %.1f V\n", VOLTAJE_BATERIA);
    if (c->numResistencias == 0) {
        printf("Resistencias: ninguna\n");
    } else {
        printf("Resistencias (%d): ", c->numResistencias);
        for (int i = 0; i < c->numResistencias; i++) {
            printf("%.1f Ω", c->resistencias[i]);
            if (i < c->numResistencias - 1) printf(" + ");
        }
        printf("\n");
    }
    if (c->hayLed) {
        printf("LED: %s (Vf = %.1f V)\n", c->colorLed, c->voltajeLed);
    } else {
        printf("LED: no conectado\n");
    }
    printf("------------------------\n");
}

void mostrarMenu() {
    printf("\n==============================================\n");
    printf("      LABORATORIO VIRTUAL - PROTOBOARD\n");
    printf("==============================================\n");
    printf("1. Ver componentes disponibles\n");
    printf("2. Agregar resistencia\n");
    printf("3. Agregar LED\n");
    printf("4. Conectar circuito y ver resultado\n");
    printf("5. Reiniciar circuito (quitar todo)\n");
    printf("6. Eliminar una resistencia\n");
    printf("7. Mostrar circuito actual\n");
    printf("8. Asistente IA: sugerir resistencia ideal\n");
    printf("9. Salir\n");
    printf("==============================================\n");
    printf("Elige una opcion: ");
}

void mostrarComponentes() {
    printf("\n--- Componentes disponibles ---\n");
    printf("Bateria: 9V (fija, siempre disponible)\n");
    printf("Resistencias: cualquier valor en ohms (ej. 220, 1000)\n");
    printf("LED: rojo, verde, amarillo, azul\n");
    printf("(Los colores no distinguen mayusculas/minusculas)\n");
}

void agregarResistencia(Circuito *c) {
    if (c->numResistencias >= MAX_RESISTENCIAS) {
        printf("\nNo puedes agregar mas resistencias (maximo %d).\n", MAX_RESISTENCIAS);
        return;
    }
    double valor;
    printf("\nIngresa el valor de la resistencia en ohms: ");
    if (!leerDouble(&valor) || valor <= 0) {
        printf("Valor invalido. Debe ser un numero positivo.\n");
        return;
    }
    c->resistencias[c->numResistencias] = valor;
    c->numResistencias++;
    printf("Resistencia de %.1f Ω agregada.\n", valor);
    mostrarCircuito(c);
}

void agregarLed(Circuito *c) {
    if (c->hayLed) {
        printf("\nYa hay un LED conectado. Reinicia o elimina el actual.\n");
        return;
    }
    char color[15];
    printf("\nElige color de LED (rojo, verde, amarillo, azul): ");
    if (!leerString(color, sizeof(color))) {
        printf("Error al leer el color.\n");
        return;
    }
    /* Convertir a minúsculas para comparar */
    aMinusculas(color);
    double vf = voltajeSegunColor(color);
    if (vf < 0) {
        printf("Color no reconocido. Usa: rojo, verde, amarillo o azul.\n");
        return;
    }
    strncpy(c->colorLed, color, sizeof(c->colorLed) - 1);
    c->colorLed[sizeof(c->colorLed) - 1] = '\0';
    c->voltajeLed = vf;
    c->hayLed = 1;
    printf("LED %s agregado (Vf = %.1f V).\n", color, vf);
    mostrarCircuito(c);
    asistenteSugerirResistencia(c);
}

void eliminarResistencia(Circuito *c) {
    if (c->numResistencias == 0) {
        printf("\nNo hay resistencias para eliminar.\n");
        return;
    }
    int idx;
    printf("\nResistencias actuales:\n");
    for (int i = 0; i < c->numResistencias; i++) {
        printf("%d: %.1f Ω\n", i+1, c->resistencias[i]);
    }
    printf("Elige el numero de la resistencia a eliminar (1-%d): ", c->numResistencias);
    int op;
    if (scanf("%d", &op) != 1 || op < 1 || op > c->numResistencias) {
        printf("Opcion invalida.\n");
        limpiarBuffer();
        return;
    }
    limpiarBuffer(); /* Consumir el salto de línea después de una entrada correcta */
    /* Desplazar hacia la izquierda */
    for (int i = op - 1; i < c->numResistencias - 1; i++) {
        c->resistencias[i] = c->resistencias[i+1];
    }
    c->numResistencias--;
    printf("Resistencia eliminada.\n");
    mostrarCircuito(c);
}

void conectarCircuito(Circuito *c) {
    printf("\n--- Conectando circuito ---\n");

    if (!c->hayLed) {
        printf("No hay LED en el circuito. Agrega uno antes de conectar.\n");
        return;
    }

    double rTotal = 0;
    for (int i = 0; i < c->numResistencias; i++)
        rTotal += c->resistencias[i];

    if (rTotal <= 0) {
        printf("PELIGRO: no hay resistencia (corto circuito).\n");
        printf("El LED %s se ha quemado instantaneamente.\n", c->colorLed);
        return;
    }

    double vDisponible = VOLTAJE_BATERIA - c->voltajeLed;
    if (vDisponible <= 0) {
        printf("El voltaje del LED (%.1f V) es mayor o igual al de la bateria (%.1f V).\n",
               c->voltajeLed, VOLTAJE_BATERIA);
        printf("No puede encender.\n");
        return;
    }

    double corriente = calcularCorriente(c);
    /* Validación extra por seguridad */
    if (corriente < 0) {
        printf("Error inesperado en el calculo de corriente.\n");
        return;
    }

    printf("Bateria: %.1f V\n", VOLTAJE_BATERIA);
    printf("Resistencia total: %.1f Ω\n", rTotal);
    printf("Voltaje directo del LED (%s): %.1f V\n", c->colorLed, c->voltajeLed);
    printf("Formula: I = (V_bat - Vf) / R_total = (%.1f - %.1f) / %.1f = %.2f mA\n",
           VOLTAJE_BATERIA, c->voltajeLed, rTotal, corriente * 1000);

    if (corriente > CORRIENTE_MAX) {
        printf("\nRESULTADO: El LED %s se ha QUEMADO (corriente = %.2f mA > %.0f mA maximo).\n",
               c->colorLed, corriente * 1000, CORRIENTE_MAX * 1000);
        printf("Sugerencia: usa una resistencia mas grande.\n");
    } else if (corriente < CORRIENTE_MIN) {
        printf("\nRESULTADO: El LED %s casi no enciende (corriente = %.2f mA < %.0f mA minimo).\n",
               c->colorLed, corriente * 1000, CORRIENTE_MIN * 1000);
        printf("Sugerencia: usa una resistencia mas pequena.\n");
    } else {
        printf("\nRESULTADO: El LED %s ENCIENDE correctamente. Buen trabajo!\n", c->colorLed);
        printf("Corriente = %.2f mA (dentro del rango seguro %.0f - %.0f mA).\n",
               corriente * 1000, CORRIENTE_MIN * 1000, CORRIENTE_MAX * 1000);
    }
}

void reiniciarCircuito(Circuito *c) {
    c->numResistencias = 0;
    c->hayLed = 0;
    c->colorLed[0] = '\0';
    c->voltajeLed = 0;
    printf("\nCircuito reiniciado. La protoboard esta vacia.\n");
}

/* ---------- Programa principal ---------- */

int main() {
    Circuito circuito;
    reiniciarCircuito(&circuito);

    /* Ordenar los valores comerciales una sola vez al inicio, para que
       lower_bound (busqueda binaria) funcione correctamente. */
    ordenarInsercion(valoresComerciales, NUM_VALORES_COMERCIALES);

    int opcion;
    int salir = 0;

    printf("Bienvenido al Laboratorio Virtual de Electronica\n");
    printf("Arma un circuito con bateria de 9V, resistencias y un LED.\n");

    while (!salir) {
        mostrarMenu();
        int resultado = scanf("%d", &opcion);
        if (resultado == EOF) {
            /* Se cerro la entrada (stdin). Salir en vez de repetir para siempre. */
            printf("\nEntrada cerrada. Saliendo del laboratorio virtual.\n");
            break;
        }
        if (resultado != 1) {
            printf("Entrada invalida. Introduce un numero.\n");
            limpiarBuffer();
            continue;
        }
        limpiarBuffer();  /* Consumir el salto de línea */

        switch (opcion) {
            case 1:
                mostrarComponentes();
                break;
            case 2:
                agregarResistencia(&circuito);
                break;
            case 3:
                agregarLed(&circuito);
                break;
            case 4:
                conectarCircuito(&circuito);
                break;
            case 5:
                reiniciarCircuito(&circuito);
                mostrarCircuito(&circuito);
                break;
            case 6:
                eliminarResistencia(&circuito);
                break;
            case 7:
                mostrarCircuito(&circuito);
                break;
            case 8:
                asistenteSugerirResistencia(&circuito);
                break;
            case 9:
                printf("\nGracias por usar el laboratorio virtual. Hasta luego!\n");
                salir = 1;
                break;
            default:
                printf("\nOpcion no valida, intenta de nuevo.\n");
        }
    }
    return 0;
}