#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/HwiP.h>
#include <kernel/nortos/dpl/common/printf.h>





void putchar_(char character)
{
    /* Output to CCS console */
    putchar(character);
    // DebugP_uartLogWriterPutChar(character);
}
