#include "HashFunctions.h"
#include "WorkDescription.h"
#include "ExecEngineData.h"
#include "Chunk.h"
#include "DataTypes.h"
#include "MMappedStorage.h"
#include "ColumnIterator.h"
#include "ColumnIterator.cc"
#include "BString.h"
#include "BStringIterator.h"
#include "Constants.h"
#include <limits.h>
#include "QueryManager.h"
#include <string.h>
#include "Logging.h" // for profiling facility
#include "Profiling.h"
#include "WPFExitCodes.h"
#include "HashFunctions.h"

extern "C"
int CleanerWorkFunc_cleaner (WorkDescription &workDescription, ExecEngineData &result) {
    return 1;
}
