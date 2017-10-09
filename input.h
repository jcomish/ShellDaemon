#ifndef INPUT_H
#define	INPUT_H

bool* validateInput(char *input);
void append(char *string, char c);
void addString(char **commandList, int currentCommand, char *rawInput, int startIndex, int endIndex);
char ** splitInput(char *input, bool ISDEBUG);
bool validateStringAmnt(char ** strings);
bool handleError(bool* isValid);
void trimInput(char *rawInput, bool ISDEBUG);
void printCommands(char ***commands);
bool isToken(char command);
void addCommand(char ***commandList, char **argList, int pos, bool ISDEBUG);
char *** splitIntoCommands(char **strings, bool ISDEBUG);
bool areCommandsValid(char **commands);
int containsCommand(char ***commands, char *command);
int countArgs(char **args);
bool containsCharacter(char *args, char character);
void freeCString(char *string);
void freeArgs(char ** string);
void freeCommandList(char *** commandList);
void clearBuffer(char * buf);
int countBufferSize(char * buf);


#endif	/* INPUT_H */

