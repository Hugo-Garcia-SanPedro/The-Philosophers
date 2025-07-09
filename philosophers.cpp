/*
    * Author: Hugo Garcia San Pedro
    * Date: 09/07/2025
    * University of Salamanca
*/

#include "windows.h"
#include "filosofar2.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <tchar.h>
#include <fstream>

// Define pointer types for DLL functions
typedef int(__cdecl* TFI2_inicio)(int, unsigned long long, struct DatosSimulaciOn*, int const*);
typedef int(__cdecl* TFI2_inicioFilOsofo)(int);
typedef int(__cdecl* TFI2_pausaAndar)(void);
typedef int(__cdecl* TFI2_puedoAndar)(void);
typedef int(__cdecl* TFI2_aDOndeVoySiAndo)(int*, int*);
typedef int(__cdecl* TFI2_andar)(void);
typedef int(__cdecl* TFI2_entrarAlComedor)(int);
typedef int(__cdecl* TFI2_cogerTenedor)(int);
typedef int(__cdecl* TFI2_comer)(void);
typedef int(__cdecl* TFI2_dejarTenedor)(int);
typedef int(__cdecl* TFI2_entrarAlTemplo)(int);
typedef int(__cdecl* TFI2_meditar)(void);
typedef int(__cdecl* TFI2_finFilOsofo)(void);
typedef int(__cdecl* TFI2_fin)(void);
typedef void(__cdecl* Tpon_error)(char*);

// Global function pointers
TFI2_inicio FI2_inicio; // Pointer to the function that initializes the simulation
TFI2_inicioFilOsofo FI2_inicioFilOsofo; // Pointer to the function that initializes a philosopher
TFI2_pausaAndar FI2_pausaAndar;  // Pointer to the function that pauses before moving
TFI2_puedoAndar FI2_puedoAndar; // Pointer to the function that checks if the philosopher can move
TFI2_andar FI2_andar; // Pointer to the function that performs the move
TFI2_entrarAlComedor FI2_entrarAlComedor; // Pointer to the function that enters the dining room
TFI2_cogerTenedor FI2_cogerTenedor;  // Pointer to the function that picks up a fork
TFI2_comer FI2_comer; // Pointer to the function that performs the eating action
TFI2_dejarTenedor FI2_dejarTenedor;  // Pointer to the function that puts down a fork
TFI2_entrarAlTemplo FI2_entrarAlTemplo;  // Pointer to the function that enters the temple
TFI2_meditar FI2_meditar; // Pointer to the function that performs the meditation action
TFI2_finFilOsofo FI2_finFilOsofo; // Pointer to the function that finalizes a philosopher
TFI2_fin FI2_fin;  // Pointer to the function that finalizes the simulation
TFI2_aDOndeVoySiAndo FI2_aDOndeVoySiAndo; // Pointer to the function that determines the next move coordinates

// Semaphores and critical sections
CRITICAL_SECTION cs_bridge;
CRITICAL_SECTION cs_shared_memory_dining;
CRITICAL_SECTION cs_temple;
CRITICAL_SECTION cs_walk;
HANDLE sem_start;
HANDLE hFileMapping;
HANDLE sem_all_walked;
HANDLE sem_bridge;
HANDLE sem_lobby;
HANDLE sem_dining;
HANDLE sem_temple;
HANDLE mutex_move[80][20];
HANDLE mutex_forks[8];

// Structs used throughout the program
struct shared_memory {
    int total_walked = 0;
    int bridge_direction;
    int dining_state[8];
    int temple_state[12];
};
typedef struct parameters {
    int num_philosophers = 0; 
    int num_rounds = 0; 
    int speed = 0;
    unsigned long long key = 28545168465992;
} Parameters;

shared_memory *shared_ptr = nullptr;
Parameters sim_params;
DatosSimulaciOn dss;

// Function prototypes
int load_Functions(HMODULE dll);
int create_IPCS(void);
void cleanup(void);
void move2(int *prev_zone, int *zone);
void move(int *prev_zone, int *zone);
int wait_Semaphore(HANDLE sem);
int release_Semaphore(HANDLE sem);
int create_Semaphore(HANDLE& sem, LONG initial, LONG maximum);
void manage_Forks(int seat, bool right_first);
void perform_Move(int* prev_zone, int* zone, int& x, int& y);
DWORD WINAPI philosopher_cycle(LPVOID lpParam); 

int main(int argc, char **argv) {
    // Check parameters
    if(argc != 4) {
        fprintf(stderr, "Parameter1 = Number of philosophers, parameter2 = Number of rounds, parameter3 = Speed.\n");
        exit(1);
    } else {
        if(atoi(argv[1]) < 1 || atoi(argv[1]) > 64) {
            fprintf(stderr, "Number of philosophers must be between 1 and 64.\n");
            exit(1);
        }
        if(atoi(argv[2]) < 1) {
            fprintf(stderr, "Rounds must be greater than 0.\n");
            exit(1);
        }
        if(atoi(argv[3]) < 0) {
            fprintf(stderr, "Speed must be 0 or greater.\n");
            exit(1);
        }
    }
    sim_params.num_philosophers = atoi(argv[1]);
    sim_params.num_rounds = atoi(argv[2]);
    sim_params.speed = atoi(argv[3]);
    // Load DLL from path
    const char *dllPath = "";
    HMODULE dll = LoadLibraryA(TEXT(dllPath));
    if(!dll) {
        fprintf(stderr, "Error loading DLL.\n");
        return 1;
    }
    // Load functions
    if(load_Functions(dll) == 1) {
        fprintf(stderr, "Error loading functions.\n");
        FreeLibrary(dll);
        return 1;
    }
    // Start simulation
    if(FI2_inicio(sim_params.speed, sim_params.key, &dss, NULL) != 0) {
        fprintf(stderr, "Error starting simulation.\n");
        FreeLibrary(dll);
        return 1;
    }
    // Create semaphores and shared memory
    if(create_IPCS() == 1) {
        fprintf(stderr, "Error creating IPCs.\n");
        return 1;
    }
    // Create threads
    std::vector<HANDLE>threads;
    for(int i = 0; i < sim_params.num_philosophers; i++) {
        HANDLE thread = CreateThread(NULL, 0, philosopher_cycle, (LPVOID)(intptr_t)i, 0, NULL);
        if(!thread) {
            fprintf(stderr, "Error creating philosopher thread.\n");
            cleanup();
            return 1;
        }
        threads.push_back(thread);
    }
    for(int i = 0; i < sim_params.num_philosophers; i++) {
        if (!(ReleaseSemaphore(sem_start, 1, NULL))) {
            fprintf(stderr, "Error releasing semaphore.\n");
            cleanup();
            return EXIT_FAILURE;
        }
    }
    // Wait for threads to finish
    WaitForMultipleObjects(threads.size(), threads.data(), TRUE, INFINITE);
    for(HANDLE h : threads) {
        CloseHandle(h);
    } 
    // Cleanup semaphores and memory
    cleanup();
    FreeLibrary(dll);
    return 0;
}

// Philosopher cycle
DWORD WINAPI philosopher_cycle(LPVOID lpParam) {
    int zone = 0, prev_zone = 0, rounds = 0, x = 1, y = 1, xPrev, yPrev, dirPhilo = 1;
    int temple_seat, checkedTemple, dining_seat, checkedDining;
    bool switchedToMove2 = FALSE, hasWalked = FALSE;

    shared_ptr->bridge_direction = 0;
    for(int i = 0; i < 8; i++) {
        shared_ptr->dining_state[i] = 0;
    }
    
    int id = (int)(intptr_t)lpParam;
    FI2_inicioFilOsofo(id);
    if(wait_Semaphore(sem_start) == 1) return EXIT_FAILURE;

    while (rounds < sim_params.num_rounds) {
        FI2_pausaAndar();
        xPrev = x;
        yPrev = y;
        FI2_aDOndeVoySiAndo(&x, &y);
        WaitForSingleObject(mutex_move[x][y], INFINITE);

        if(!switchedToMove2) {
            move(&prev_zone, &zone);
            switchedToMove2 = TRUE;
            if(!hasWalked) {
                shared_ptr->total_walked++;
                hasWalked = TRUE;
            }
            if(shared_ptr->total_walked == sim_params.num_philosophers) {
                if(release_Semaphore(sem_all_walked) == 1) return EXIT_FAILURE;
            }
        } else {
            move2(&prev_zone, &zone);
        }
        ReleaseMutex(mutex_move[xPrev][yPrev]);

        switch (zone) {
            // Field zone
            case CAMPO:
                if(prev_zone == PUENTE) {
                    perform_Move(&prev_zone, &zone, x, y);
                    dirPhilo = -dirPhilo;
                    shared_ptr->bridge_direction += dirPhilo;
                    if(release_Semaphore(sem_bridge) == 1) return EXIT_FAILURE;
                }
                if(prev_zone == SITIOTEMPLO) {
                    if(release_Semaphore(sem_temple) == 1) return EXIT_FAILURE;
                }
                break;

            // Bridge zone
            case PUENTE:
                if(prev_zone == CAMPO) {
                    if(shared_ptr->total_walked < sim_params.num_philosophers) {
                        if(wait_Semaphore(sem_all_walked) == 1) return EXIT_FAILURE;
                    }
                    while(true) {
                        if(wait_Semaphore(sem_bridge) == 1) return EXIT_FAILURE;
                        EnterCriticalSection(&cs_bridge);
                        if(!(shared_ptr->bridge_direction != 0 && (shared_ptr->bridge_direction * dirPhilo) < 0)) {
                            shared_ptr->bridge_direction += dirPhilo;
                            LeaveCriticalSection(&cs_bridge);
                            break;
                        }
                        LeaveCriticalSection(&cs_bridge);
                        if(release_Semaphore(sem_bridge) == 1) return EXIT_FAILURE;
                    }
                }
                break;

            // Lobby zone
            case ANTESALA:
                if(prev_zone == CAMPO) {
                    if(wait_Semaphore(sem_lobby) == 1) return EXIT_FAILURE;
                }
                break;

            // Dining entrance zone
            case ENTRADACOMEDOR:
                if(prev_zone == ANTESALA) {
                    if(wait_Semaphore(sem_dining) == 1) return EXIT_FAILURE;
                    EnterCriticalSection(&cs_shared_memory_dining);
                    dining_seat = checkedDining = 0;
                    while(checkedDining < 8) {
                        if(shared_ptr->dining_state[dining_seat] == 0) {
                            break;
                        }
                        dining_seat += 3;
                        if(dining_seat >= 8) {
                            dining_seat -= 8;
                        }
                        checkedDining++;
                    }
                    if(checkedDining < 8) {
                        shared_ptr->dining_state[dining_seat] = 1;
                        FI2_entrarAlComedor(dining_seat);
                        perform_Move(&prev_zone, &zone, x, y);
                        if(release_Semaphore(sem_lobby) == 1) return EXIT_FAILURE;
                        LeaveCriticalSection(&cs_shared_memory_dining);
                    } else {
                        LeaveCriticalSection(&cs_shared_memory_dining);
                    }
                }
                break;

            // Dining chair zone
            case SILLACOMEDOR:
                manage_Forks(dining_seat, dining_seat % 2 == 0);
                EnterCriticalSection(&cs_shared_memory_dining);
                shared_ptr->dining_state[dining_seat] = 0;
                LeaveCriticalSection(&cs_shared_memory_dining);
                break;

            // Dining exit zone
            case SALIDACOMEDOR:
                if(prev_zone == SILLACOMEDOR) {
                    if(release_Semaphore(sem_dining) == 1) return EXIT_FAILURE;
                }
                break;

            // Temple zone
            case TEMPLO:
                if(prev_zone == CAMPO) {
                    if(wait_Semaphore(sem_temple) == 1) return EXIT_FAILURE;
                    EnterCriticalSection(&cs_temple);
                    temple_seat = checkedTemple = 0;
                    while(checkedTemple < 12) {
                        if(shared_ptr->temple_state[temple_seat] == 0) {
                            break;
                        }
                        temple_seat++;
                        if(temple_seat == 12) {
                            temple_seat = 0;
                        }
                        checkedTemple++;
                    }
                    if(checkedTemple < 12) {
                        shared_ptr->temple_state[temple_seat] = 1;
                        FI2_entrarAlTemplo(temple_seat);
                        LeaveCriticalSection(&cs_temple);
                    } else {
                        LeaveCriticalSection(&cs_temple);
                    }
                }
                break;

            // Temple seat zone
            case SITIOTEMPLO:
                while(FI2_meditar() == SITIOTEMPLO);
                EnterCriticalSection(&cs_temple);
                shared_ptr->temple_state[temple_seat] = 0;
                LeaveCriticalSection(&cs_temple);
                rounds++;
                break;
        }
    }
    if(release_Semaphore(sem_temple) == 1) return EXIT_FAILURE;
    FI2_finFilOsofo();
    return 0;
}

// Function to manage forks
void manage_Forks(int seat, bool right_first) {
    int left = seat;
    int right = (seat == 0) ? 7 : (seat - 1);

    if(right_first) {
        WaitForSingleObject(mutex_forks[right], INFINITE);
        FI2_cogerTenedor(TENEDORDERECHO);
        WaitForSingleObject(mutex_forks[left], INFINITE);
        FI2_cogerTenedor(TENEDORIZQUIERDO);
    } else {
        WaitForSingleObject(mutex_forks[left], INFINITE);
        FI2_cogerTenedor(TENEDORIZQUIERDO);
        WaitForSingleObject(mutex_forks[right], INFINITE);
        FI2_cogerTenedor(TENEDORDERECHO);
    }

    while (FI2_comer() == SILLACOMEDOR);
    FI2_dejarTenedor(TENEDORIZQUIERDO);
    ReleaseMutex(mutex_forks[left]);
    FI2_dejarTenedor(TENEDORDERECHO);
    ReleaseMutex(mutex_forks[right]);
}

// Function to perform the move
void perform_Move(int* prev_zone, int* zone, int& x, int& y) {
    int xPrev = x;
    int yPrev = y;

    FI2_aDOndeVoySiAndo(&x, &y);
    WaitForSingleObject(mutex_move[x][y], INFINITE);
    move2(prev_zone, zone);
    ReleaseMutex(mutex_move[xPrev][yPrev]);
}

// Function to load DLL functions
int load_Functions(HMODULE dll) {
    FI2_inicio = (TFI2_inicio)GetProcAddress(dll, "FI2_inicio");
    FI2_inicioFilOsofo = (TFI2_inicioFilOsofo)GetProcAddress(dll, "FI2_inicioFilOsofo");
    FI2_pausaAndar = (TFI2_pausaAndar)GetProcAddress(dll, "FI2_pausaAndar");
    FI2_puedoAndar = (TFI2_puedoAndar)GetProcAddress(dll, "FI2_puedoAndar");
    FI2_andar = (TFI2_andar)GetProcAddress(dll, "FI2_andar");
    FI2_entrarAlComedor = (TFI2_entrarAlComedor)GetProcAddress(dll, "FI2_entrarAlComedor");
    FI2_cogerTenedor = (TFI2_cogerTenedor)GetProcAddress(dll, "FI2_cogerTenedor");
    FI2_comer = (TFI2_comer)GetProcAddress(dll, "FI2_comer");
    FI2_dejarTenedor = (TFI2_dejarTenedor)GetProcAddress(dll, "FI2_dejarTenedor");
    FI2_entrarAlTemplo = (TFI2_entrarAlTemplo)GetProcAddress(dll, "FI2_entrarAlTemplo");
    FI2_meditar = (TFI2_meditar)GetProcAddress(dll, "FI2_meditar");
    FI2_finFilOsofo = (TFI2_finFilOsofo)GetProcAddress(dll, "FI2_finFilOsofo");
    FI2_fin = (TFI2_fin)GetProcAddress(dll, "FI2_fin");
    FI2_aDOndeVoySiAndo = (TFI2_aDOndeVoySiAndo)GetProcAddress(dll, "FI2_aDOndeVoySiAndo");

    // Check for errors loading functions
    if (!FI2_inicio) { fprintf(stderr, "Could not load FI2_inicio\n"); return 1; }
    if (!FI2_inicioFilOsofo) { fprintf(stderr, "Could not load FI2_inicioFilOsofo\n"); return 1; }
    if (!FI2_pausaAndar) { fprintf(stderr, "Could not load FI2_pausaAndar\n"); return 1; }
    if (!FI2_puedoAndar) { fprintf(stderr, "Could not load FI2_puedoAndar\n"); return 1; }
    if (!FI2_andar) { fprintf(stderr, "Could not load FI2_andar\n"); return 1; }
    if (!FI2_entrarAlComedor) { fprintf(stderr, "Could not load FI2_entrarAlComedor\n"); return 1; }
    if (!FI2_cogerTenedor) { fprintf(stderr, "Could not load FI2_cogerTenedor\n"); return 1; }
    if (!FI2_comer) { fprintf(stderr, "Could not load FI2_comer\n"); return 1; }
    if (!FI2_dejarTenedor) { fprintf(stderr, "Could not load FI2_dejarTenedor\n"); return 1; }
    if (!FI2_entrarAlTemplo) { fprintf(stderr, "Could not load FI2_entrarAlTemplo\n"); return 1; }
    if (!FI2_meditar) { fprintf(stderr, "Could not load FI2_meditar\n"); return 1; }
    if (!FI2_finFilOsofo) { fprintf(stderr, "Could not load FI2_finFilOsofo\n"); return 1; }
    if (!FI2_fin) { fprintf(stderr, "Could not load FI2_fin\n"); return 1; }
    if (!FI2_aDOndeVoySiAndo) { fprintf(stderr, "Could not load FI2_aDOndeVoySiAndo\n"); return 1; }
    
    return 0;
}

// Function to create semaphores
int create_Semaphore(HANDLE& sem, LONG initial, LONG maximum) {
    sem = CreateSemaphore(NULL, initial, maximum, NULL);
    if(sem == NULL) {
        fprintf(stderr, "Error creating semaphore.\n");
        return 1;
    }
    return 0;
}

// Function to wait for a semaphore
int wait_Semaphore(HANDLE sem) {
    if (WaitForSingleObject(sem, INFINITE) != WAIT_OBJECT_0) {
        fprintf(stderr, "Error waiting for semaphore\n");
        return 1;
    }
    return 0;
}

// Function to release a semaphore
int release_Semaphore(HANDLE sem) {
    if (!ReleaseSemaphore(sem, 1, NULL)) {
        fprintf(stderr, "Error releasing semaphore\n");
        return 1;
    }
    return 0;
}

// Cleanup function
void cleanup(void) {
    UnmapViewOfFile(shared_ptr);
    CloseHandle(hFileMapping);
    CloseHandle(sem_start);
    CloseHandle(sem_bridge);
    CloseHandle(sem_lobby);
    CloseHandle(sem_dining);
    CloseHandle(sem_temple);
    CloseHandle(mutex_move);
    CloseHandle(mutex_forks);
    DeleteCriticalSection(&cs_bridge);
    DeleteCriticalSection(&cs_shared_memory_dining);
    DeleteCriticalSection(&cs_temple);
    DeleteCriticalSection(&cs_walk);
    FI2_fin();
}

// Function to create semaphores and shared memory
int create_IPCS(void) {
    InitializeCriticalSection(&cs_bridge);
    InitializeCriticalSection(&cs_shared_memory_dining);
    InitializeCriticalSection(&cs_temple);
    InitializeCriticalSection(&cs_walk);

    if(create_Semaphore(sem_bridge, dss.maxFilOsofosEnPuente, dss.maxFilOsofosEnPuente) == 1) return 1;
    if(create_Semaphore(sem_start, 0, sim_params.num_philosophers) == 1) return 1;
    if(create_Semaphore(sem_lobby, 4, 4) == 1) return 1;
    if(create_Semaphore(sem_dining, dss.nTenedores, dss.nTenedores) == 1) return 1;
    if(create_Semaphore(sem_all_walked, 0, 1) == 1) return 1;
    if(create_Semaphore(sem_temple, dss.sitiosTemplo, dss.sitiosTemplo) == 1) return 1;
    
    for(int i = 0; i < 8; i++) {
        mutex_forks[i] = CreateMutex(NULL, FALSE, NULL);
        if(mutex_forks[i] == NULL) {
            fprintf(stderr, "Error creating mutex.\n");
            return 1;
        }
    }
    for(int i = 0; i < 90; i++) {
        for (int j = 0; j < 30; j++) {
            mutex_move[i][j] = CreateMutex(NULL, FALSE, NULL);
            if(mutex_move[i][j] == NULL) {
                fprintf(stderr, "Error creating mutex.\n");
                return 1;
            }
        }
    }

    hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(shared_memory), NULL);
    if(hFileMapping == NULL) {
        fprintf(stderr, "Error creating file mapping.\n");
        return 1;
    }
    shared_ptr = (shared_memory*)MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shared_memory));
    if(shared_ptr == nullptr) {
        fprintf(stderr, "Error mapping memory.\n");
        CloseHandle(hFileMapping);
        return 1;
    }
    return 0;
}

// Functions to move
void move(int *prev_zone, int *zone) {
    while(TRUE) {
        EnterCriticalSection(&cs_walk);
        if(FI2_puedoAndar() == 100) {
            *prev_zone = *zone;
            *zone = FI2_andar();
            LeaveCriticalSection(&cs_walk);
            return;
        } else {
            LeaveCriticalSection(&cs_walk);
        }
    }
}

void move2(int* prev_zone, int* zone) {
    *prev_zone = *zone;
    *zone = FI2_andar();
}