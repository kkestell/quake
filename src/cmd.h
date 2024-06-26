
// cmd.h -- Command buffer and command execution

//===========================================================================

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

void Cbuf_Init(void);
// allocates an initial text buffer that will grow as needed

void Cbuf_AddText(char *text);
// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.

void Cbuf_InsertText(char *text);
// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.

void Cbuf_Execute(void);
// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

Commands can come from three sources, but the handler functions may choose
to dissallow the action or forward it to a remote server if the source is
not apropriate.

*/

typedef void (*xcommand_t)(void);

typedef enum
{
    src_client, // came in over a net connection as a clc_stringcmd
                // host_client will be valid during this state.
    src_command // from the command buffer
} cmd_source_t;

extern cmd_source_t cmd_source;

void Cmd_Init(void);

void Cmd_AddCommand(char *cmd_name, xcommand_t function);
// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory

bool Cmd_Exists(char *cmd_name);
// used by the cvar code to check for cvar / command name overlap

char *Cmd_CompleteCommand(char *partial);
// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits

int32_t Cmd_Argc(void);
char *Cmd_Argv(int32_t arg);
char *Cmd_Args(void);
// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are allways safe.

int32_t Cmd_CheckParm(char *parm);
// Returns the position (1 to argc-1) in the command's argument list
// where the given parameter apears, or 0 if not present

void Cmd_TokenizeString(char *text);
// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.

void Cmd_ExecuteString(char *text, cmd_source_t src);
// Parses a single line of text into arguments and tries to execute it.
// The text can come from the command buffer, a remote client, or stdin.

void Cmd_ForwardToServer(void);
// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.

void Cmd_Print(char *text);
// used by command functions to send output to either the graphics console or
// passed as a print message to the client
