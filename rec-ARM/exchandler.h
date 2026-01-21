//

#ifndef __exchandler_h
#define __exchandler_h


/*
**
*/

#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <exception>
#include <iostream>

using namespace std;

/*

#include <execinfo.h>

class ExceptionTracer
{
public:
    ExceptionTracer()
    {
        void * array[25];
        int nSize = backtrace(array, 25);
        char ** symbols = backtrace_symbols(array, nSize);

        for (int i = 0; i < nSize; i++) {
            cout << "Sym[" << i << "] " << symbols[i] << endl;
        }

        free(symbols);
    }
};




 template <class SignalExceptionClass> class SignalTranslator
 {
 private:
     class SingleTonTranslator
     {
     public:
         SingleTonTranslator()
         {
             signal(SignalExceptionClass::GetSignalNumber(), SignalHandler);
         }

         static void SignalHandler(int)
         {
             throw SignalExceptionClass();
         }
     };

 public:
     SignalTranslator()
     {
         static SingleTonTranslator s_objTranslator;
     }
 };






 class SegmentationFault : public ExceptionTracer, public exception
 {
 public:
     static int GetSignalNumber() {return SIGSEGV;}
 };

 SignalTranslator<SegmentationFault> g_objSegmentationFaultTranslator;



 class FloatingPointException : public ExceptionTracer, public exception
 {
 public:
     static int GetSignalNumber() {return SIGFPE;}
 };

 SignalTranslator<FloatingPointException> g_objFloatingPointExceptionTranslator;















 class ExceptionHandler
 {
 private:
     class SingleTonHandler
     {
     public:
         SingleTonHandler()
         {
             set_terminate(Handler);
         }

         static void Handler()
         {
             // Exception from construction/destruction of global variables
             try
             {
                 // re-throw
                 throw;
             }
             catch (SegmentationFault &)
             {
                 cout << "SegmentationFault" << endl;
             }
             catch (FloatingPointException &)
             {
                 cout << "FloatingPointException" << endl;
             }
             catch (...)
             {
                 cout << "Unknown Exception" << endl;
             }
             //if this is a thread performing some core activity
             abort();
             // else if this is a thread used to service requests
             // pthread_exit();
         }
     };

 public:
     ExceptionHandler()
     {
         static SingleTonHandler s_objHandler;
     }
 };
*/


#endif
