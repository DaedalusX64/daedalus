//*****************************************************************************
// The Command and Help topic maps
//*****************************************************************************
#define BEGIN_DBGCOMMAND_MAP(name) \
const IDebugConsole::DebugCommandInfo IDebugConsole::name[] =	\
{
#define DBGCOMMAND_ARGLESS(cmd, func) \
{ cmd, func, NULL, NULL },
#define DBGCOMMAND_ARG(cmd, func) \
{ cmd, NULL, func, NULL },

#define DBGCOMMAND_ARGLESS_HELP(cmd, func, help) \
{ cmd, func, NULL, help },
#define DBGCOMMAND_ARG_HELP(cmd, func, help) \
{ cmd, NULL, func, help },

#define BEGIN_DBGCOMMAND_ARGLESS_HELP(cmd, func) \
{ cmd, func, NULL,
#define BEGIN_DBGCOMMAND_ARG_HELP(cmd, func) \
{ cmd, NULL, func,

#define BEGIN_DBGCOMMAND() },

#define END_DBGCOMMAND_MAP()		\
	{ NULL, NULL }					\
};

#define BEGIN_HELPTOPIC_MAP(name) \
const IDebugConsole::HelpTopicInfo IDebugConsole::name[] =	\
{

#define HELPTOPIC(topic, text) \
{ topic, text },

#define BEGIN_HELPTOPIC(topic) { topic,

#define END_HELPTOPIC() },

#define END_HELPTOPIC_MAP()		\
	{ NULL, NULL }				\
};

BEGIN_DBGCOMMAND_MAP(mDebugCommands)
	DBGCOMMAND_ARG_HELP( "help", &IDebugConsole::CommandHelp, "Displays commands and help topics or shows help text for one of them" ) // Implemented by Lkb
	DBGCOMMAND_ARGLESS_HELP( "go", &IDebugConsole::CommandGo, "Starts the CPU" )
	DBGCOMMAND_ARGLESS_HELP( "stop", &IDebugConsole::CommandStop, "Stops the CPU" )
	DBGCOMMAND_ARGLESS_HELP( "skip", &IDebugConsole::CommandCPUSkip, "When the CPU is stopped, skips the current instruction" )
	DBGCOMMAND_ARGLESS_HELP( "enablegfx", &IDebugConsole::CommandRDPEnableGfx, "Reenables display list processing" )
	DBGCOMMAND_ARGLESS_HELP( "disablegfx", &IDebugConsole::CommandRDPDisableGfx, "Turns off Display List processing" )
#ifdef DAEDALUS_ENABLE_DYNAREC
	DBGCOMMAND_ARGLESS( "enable dynarec", &IDebugConsole::CommandCPUDynarecEnable )
#endif
	DBGCOMMAND_ARGLESS_HELP( "int pi", &IDebugConsole::CommandIntPi, "Generates a PI interrupt" )
	DBGCOMMAND_ARGLESS_HELP( "int vi", &IDebugConsole::CommandIntVi, "Generates a VI interrupt" )
	DBGCOMMAND_ARGLESS_HELP( "int ai", &IDebugConsole::CommandIntAi, "Generates a AI interrupt" )
	DBGCOMMAND_ARGLESS_HELP( "int dp", &IDebugConsole::CommandIntDp, "Generates a DP interrupt" )
	DBGCOMMAND_ARGLESS_HELP( "int sp", &IDebugConsole::CommandIntSp, "Generates a SP interrupt" )
	DBGCOMMAND_ARGLESS_HELP( "int si", &IDebugConsole::CommandIntSi, "Generates a SI interrupt" )

	DBGCOMMAND_ARG_HELP( "dis", &IDebugConsole::CommandDis, "Dumps disassembly of specified region to disk" )
	DBGCOMMAND_ARG_HELP( "rdis", &IDebugConsole::CommandRDis, "Dumps disassembly of the rsp imem/dmem to disk" )
	DBGCOMMAND_ARG_HELP( "strings", &IDebugConsole::CommandStrings, "Dumps all the strings in the rom image to a text file" )

#ifdef DAEDALUS_ENABLE_OS_HOOKS
	DBGCOMMAND_ARG_HELP( "listos", &IDebugConsole::CommandListOS, "Shows the os symbol in the code view (e.g. __osDisableInt)" )
#endif
	DBGCOMMAND_ARG_HELP( "list", &IDebugConsole::CommandList, "Shows the specified address in the code view" )
	DBGCOMMAND_ARGLESS_HELP( "cpu", &IDebugConsole::CommandShowCPU, "Show the CPU code view" )
	DBGCOMMAND_ARG_HELP( "fp", &IDebugConsole::CommandFP, "Displays the specified floating point register" )
//	DBGCOMMAND_ARG_HELP( "vec", &IDebugConsole::CommandVec, "Dumps the specified rsp vector" )

	DBGCOMMAND_ARG_HELP( "mem", &IDebugConsole::CommandMem, "Shows the memory at the specified address" )
	DBGCOMMAND_ARG_HELP( "bpx", &IDebugConsole::CommandBPX, "Sets a breakpoint at the specified address" )
	DBGCOMMAND_ARG_HELP( "bpd", &IDebugConsole::CommandBPD, "Disables a breakpoint at the specified address" )
	DBGCOMMAND_ARG_HELP( "bpe", &IDebugConsole::CommandBPE, "Enables a breakpoint at the specified address" )

	DBGCOMMAND_ARG_HELP( "w8", &IDebugConsole::CommandWrite8, "Writes the specified 8-bit value at the specified address. Type \"help write\" for more info.") // Added and implemented by Lkb
	DBGCOMMAND_ARG_HELP( "w16", &IDebugConsole::CommandWrite16, "Writes the specified 16-bit value at the specified address. Type \"help write\" for more info." ) // Added and implemented by Lkb
	DBGCOMMAND_ARG_HELP( "w32", &IDebugConsole::CommandWrite32, "Writes the specified 32-bit value at the specified address. Type \"help write\" for more info." ) // Added and implemented by Lkb
	DBGCOMMAND_ARG_HELP( "w64", &IDebugConsole::CommandWrite64, "Writes the specified 64-bit value at the specified address. Type \"help write\" for more info." ) // Added and implemented by Lkb
	DBGCOMMAND_ARG_HELP( "wr", &IDebugConsole::CommandWriteReg, "Writes the specified value in the specified register. Type \"help write\" for more info." ) // Added and implemented by Lkb

#ifdef DUMPOSFUNCTIONS
	DBGCOMMAND_ARGLESS_HELP( "osinfo threads", &IDebugConsole::CommandPatchDumpOsThreadInfo, "Displays a list of active threads" )
	DBGCOMMAND_ARGLESS_HELP( "osinfo queues", &IDebugConsole::CommandPatchDumpOsQueueInfo, "Display a list of queues (some are usually invalid)" )
	DBGCOMMAND_ARGLESS_HELP( "osinfo events", &IDebugConsole::CommandPatchDumpOsEventInfo, "Display the system event details" )
#endif

	DBGCOMMAND_ARGLESS_HELP( "close", &IDebugConsole::CommandClose, "Closes the debug console") // Added and implemented by Lkb
	DBGCOMMAND_ARGLESS_HELP( "quit", &IDebugConsole::CommandQuit, "Quits Daedalus") // Added and implemented by Lkb
	DBGCOMMAND_ARGLESS_HELP( "exit", &IDebugConsole::CommandQuit, "Quits Daedalus") // Added and implemented by Lkb

#ifdef DAEDALUS_DEBUG_DYNAREC
	DBGCOMMAND_ARGLESS_HELP( "dumpdyna", &IDebugConsole::CommandDumpDyna, "Dump Dyna States")
#endif

END_DBGCOMMAND_MAP()


BEGIN_HELPTOPIC_MAP(mHelpTopics)

	BEGIN_HELPTOPIC("numbers")
	"In Daedalus you can specify numbers with the following notations:\n"
	"\tDecimal: 0t12345678\n"
	"\tHexadecimal: 0xabcdef12 or abcdef12\n"
	"\tRegister: %sp\n"
	"\tRegister+-value: %sp+2c\n"
	"\tDon't add spaces when using the reg+-value notation: \"%sp + 4\" is incorrect\n"
	"\tIn this release, \"mem [[sp+4[]\" is replaced by \"mem %sp+4\""
	END_HELPTOPIC()

	BEGIN_HELPTOPIC("write")
	"Syntax:\n"
	"w{8|16|32|64} <address> [[<operators>[] <value>\n"
	"wr <regname> [[<operators>[] <value>\n"
	"\n"
	"<address> - Address to modify\n"
	"<regname> - Register to modify\n"
	"<value> - Value to use\n"
	"<operators> - One or more of the following symbols:\n"
	"\t$ - Bit mode: uses (1 << <value>) instead of <value>\n"
	"\t~ - Not mode: negates <value> before using it\n"
	"\t| - Or operator: combines [[<address>[] with <value> using Or\n"
	"\t^ - Xor operator: combines [[<address>[] with <value> using Xor\n"
	"\t& - And operator: combines [[<address>[] with <value> using And\n"
	"For information on the format of <address> and <value>, type \"help numbers\""
	END_HELPTOPIC()

	BEGIN_HELPTOPIC("rspmem")
	"To read or write form RSP code memory you must replace the first zero with 'a'\n"
	"Example: to write NOP to RSP 0x04001090 use: w32 a4001090 0"
	END_HELPTOPIC()

	BEGIN_HELPTOPIC("dis")
	"Disassemble a region of memory to file\n"
	"Syntax:\n"
	"dis <address1> <address2> [[<filename>[]\n"
	"\n"
	"<address1> - Starting address\n"
	"<address2> - Ending address\n"
	"<filename> - Optional filename (default is dis.txt)\n"
	"Example: Bootrom   : dis 0xB0000000 0xB0001000\n"
	"         ExcVectors: dis 0x80000000 0x80000200"
	END_HELPTOPIC()

	BEGIN_HELPTOPIC("rdis")
	"Disassemble the rsp dmem/imem to file\n"
	"Syntax:\n"
	"dis [[<filename>[]\n"
	"\n"
	"<filename> - Optional filename (default is rdis.txt)"
	END_HELPTOPIC()

	BEGIN_HELPTOPIC("strings")
	"Dump ASCII strings to file\n"
	"Syntax:\n"
	"strings [[<filename>[]\n"
	"\n"
	"<filename> - Optional filename (default is strings.txt)"
	END_HELPTOPIC()

END_HELPTOPIC_MAP()


