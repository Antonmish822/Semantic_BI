#ifndef SAXONDEV_USERFUNCTIONHANDLER_H
#define SAXONDEV_USERFUNCTIONHANDLER_H
class XdmValue;

typedef XdmValue *(*sxnExtensionFunction)(int, XdmValue **);

typedef void *(*phpExtensionFunction)(char *, void *, void *, int, int,  void **);

typedef void *(*pythonExtensionFunction)(void *, void *, int, int, void **);

void * userFunctionNativeCall(char * functionName, void * proc, void * func, int useNativeTypes, int numOfArguments,  XdmValue ** arguments);

//void * userFunctionPHPNativeCall(char * functionName, void * proc, void * func, int useNativeTypes, int numOfArguments,  XdmValue ** arguments);

#endif //SAXONDEV_USERFUNCTIONHANDLER_H
