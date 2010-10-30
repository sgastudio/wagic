#include "PrecompiledHeader.h"

#include "Logger.h"
#ifdef DOLOG

using namespace std;

void Logger::Log(const char * text){
  ofstream file (LOG_FILE,ios_base::app);
  if (file){
    file << text;
    file << "\n";
    file.close();
  }

  DebugTrace(text);
}

#endif