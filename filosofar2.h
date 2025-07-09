#ifdef FI2_EXPORTS
#define FI2_API __declspec(dllexport)
#else
#define FI2_API __declspec(dllimport)
#endif
 
// void PERROR(char const *a) 
//  {LPVOID lpMsgBuf;
//   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
//                 FORMAT_MESSAGE_FROM_SYSTEM |
//                 FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
//                 GetLastError(),
//                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//                 (LPTSTR) &lpMsgBuf,0,NULL);
//   fprintf(stderr,"%s:%d:%s\n",a,GetLastError(),lpMsgBuf);
//   LocalFree(lpMsgBuf);}

// ZONAS
#define CAMPO            0
#define PUENTE           1
#define ANTESALA         2
#define ENTRADACOMEDOR   3
#define SILLACOMEDOR     4
#define SALIDACOMEDOR    5
#define TEMPLO           6
#define SITIOTEMPLO      7
#define CEMENTERIO       8

char const *nombreZonas[]={"CAMPO","PUENTE","ANTESALA","ENTRADACOMEDOR",
                           "SILLACOMEDOR","SALIDACOMEDOR","TEMPLO","SITIOTEMPLO",
                           "CEMENTERIO"};

#define MAXTENEDORES    21
#define MAXSITIOSTEMPLO 14
#define MAXFILOSOFOS    64

#define TENEDORDERECHO   0
#define TENEDORIZQUIERDO 1

struct DatosSimulaciOn
 {int maxFilOsofosEnPuente;  // FilOsofos como mAximo en el puente
                             //   Si es 10, sin restricciOn de peso
  int maxUnaDirecciOnPuente; // FilOsofos que pueden pasar en una direcciOn
                             // en el puente como mAximo si hay filOsofos
                             // esperando para pasar en la otra direcciOn
                             //   Si es 0, no existe esta restricciOn
  int sitiosTemplo;          // Sitios para meditar en el templo
  int nTenedores;            // NUmero de tenedores o puestos en el comedor
  };

#ifdef FI2_EXPORTS
extern "C" FI2_API int FI2_inicio(long,unsigned long long,
                                  struct DatosSimulaciOn *,int const *);
extern "C" FI2_API int FI2_inicioFilOsofo(int);
extern "C" FI2_API int FI2_pausaAndar(void);
extern "C" FI2_API int FI2_puedoAndar(void);
extern "C" FI2_API int FI2_aDOndeVoySiAndo(int *x, int *y);
extern "C" FI2_API int FI2_andar(void);
extern "C" FI2_API int FI2_entrarAlComedor(int);
extern "C" FI2_API int FI2_cogerTenedor(int);
extern "C" FI2_API int FI2_comer(void);
extern "C" FI2_API int FI2_dejarTenedor(int);
extern "C" FI2_API int FI2_entrarAlTemplo(int);
extern "C" FI2_API int FI2_meditar(void);
extern "C" FI2_API int FI2_finFilOsofo(void);
extern "C" FI2_API int FI2_fin(void);
#endif
