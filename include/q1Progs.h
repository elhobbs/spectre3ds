#pragma once
#include "quakedef.h"
#include "sys.h"
#include "pr_comp.h"
#include "NetBuffer.h"
#include "vec3_fixed.h"

class Server;

typedef struct {
	union {
		float _float;
		int		_int;
	};
} q1_val_t;

typedef struct
{
	int				s;
	dfunction_t		*f;
} prstack_t;

#define	MAX_STACK_DEPTH		32
#define	LOCALSTACK_SIZE		2048

#define	MAX_EDICTS		500			// FIXME: ouch! ouch! ouch!

typedef union eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				_int;
	int				edict;
} eval_t;


class q1Progs {
public:
		q1Progs(Server &sv);
		int load();

		Server &m_sv;

		void program_execute(func_t fnum);
		int function_enter(dfunction_t *f);
		int function_leave();

		void run_error(char *error, ...);

		eval_t *prog_to_eval(int prog);
		edict_t *prog_to_edict(int prog);
		int edict_to_prog(edict_t *e);
		edict_t* G_EDICT(int o);
		int NUM_FOR_EDICT(edict_t *e);
		edict_t * NEXT_EDICT(edict_t *e);
		edict_t *ED_Alloc(void);
		edict_t *EDICT_NUM(int n);
		edict_t *EDICT_NUM2(int n);
		void ED_ClearEdict(edict_t *e);
		edict_t * ed_create();
		void ED_Free(edict_t *ed);
		NetBuffer *WriteDest(void);

		ddef_t *ED_FindField(char *name);
		dfunction_t *ED_FindFunction(char *name);
		char *ED_NewString(char *string);
		bool ED_ParseEpair(void *base, ddef_t *key, char *s);
		eval_t *GetEdictFieldValue(edict_t *ed, char *field);

		char* valueString(int type, eval_t *val);
		ddef_t *ED_FieldAtOfs(int ofs);
		void ED_WriteGlobals(FILE *f);
		void ED_Write(FILE *f, edict_t *ed);
		void ED_Write(FILE *f);
		ddef_t *ED_FindGlobal(char *name);


		char *PF_VarString(int	first);
		void SetMinMaxSize(edict_t *e, float *min, float *max, bool rotate);
		void SetMinMaxSize(edict_t *e, vec3_fixed32 &min, vec3_fixed32 &max, bool rotate);
		int PF_newcheckclient(int check);
		void PR_CheckEmptyString(char *s);
		edict_t* find(char *classname);

		void PF_Fixme();
		void PF_makevectors();	// void(entity e)	makevectors 		= #1;
		void PF_setorigin();	// void(entity e(); vector o) setorigin	= #2;
		void PF_setmodel();	// void(entity e(); string m) setmodel	= #3;
		void PF_setsize();	// void(entity e(); vector min(); vector max) setsize = #4;
		void PF_break();	// void() break						= #6;
		void PF_random();	// float() random						= #7;
		void PF_sound();	// void(entity e(); float chan(); string samp) sound = #8;
		void PF_normalize();	// vector(vector v) normalize			= #9;
		void PF_error();	// void(string e) error				= #10;
		void PF_objerror();	// void(string e) objerror				= #11;
		void PF_vlen();	// float(vector v) vlen				= #12;
		void PF_vectoyaw();	// float(vector v) vectoyaw		= #13;
		void PF_Spawn();	// entity() spawn						= #14;
		void PF_Remove();	// void(entity e) remove				= #15;
		void PF_traceline();	// float(vector v1(); vector v2(); float tryents) traceline = #16;
		void PF_checkclient();	// entity() clientlist					= #17;
		void PF_Find();	// entity(entity start(); .string fld(); string match) find = #18;
		void PF_precache_sound();	// void(string s) precache_sound		= #19;
		void PF_precache_model();	// void(string s) precache_model		= #20;
		void PF_stuffcmd();	// void(entity client(); string s)stuffcmd = #21;
		void PF_findradius();	// entity(vector org(); float rad) findradius = #22;
		void PF_bprint();	// void(string s) bprint				= #23;
		void PF_sprint();	// void(entity client(); string s) sprint = #24;
		void PF_dprint();	// void(string s) dprint				= #25;
		void PF_ftos();	// void(string s) ftos				= #26;
		void PF_vtos();	// void(string s) vtos				= #27;
		void PF_coredump();
		void PF_traceon();
		void PF_traceoff();
		void PF_eprint();	// void(entity e) debug print an entire entity
		void PF_walkmove(); // float(float yaw(); float dist) walkmove
		void PF_droptofloor();
		void PF_lightstyle();
		void PF_rint();
		void PF_floor();
		void PF_ceil();
		void PF_checkbottom();
		void PF_pointcontents();
		void PF_fabs();
		void PF_aim();
		void PF_cvar();
		void PF_localcmd();
		void PF_nextent();
		void PF_particle();
		void PF_changeyaw();
		void PF_vectoangles();

		void PF_WriteByte();
		void PF_WriteChar();
		void PF_WriteShort();
		void PF_WriteLong();
		void PF_WriteCoord();
		void PF_WriteAngle();
		void PF_WriteString();
		void PF_WriteEntity();

		void PF_MoveToGoal();
		void PF_precache_file();
		void PF_makestatic();

		void PF_changelevel();

		void PF_cvar_set();
		void PF_centerprint();

		void PF_ambientsound();

		void PF_setspawnparms();

	globalvars_t	*m_global_struct;
	dprograms_t		m_programs;
	prstack_t		m_pr_stack[MAX_STACK_DEPTH];
	int				m_pr_depth;

	int				m_localstack[LOCALSTACK_SIZE];
	int				m_localstack_used;

	bool			m_pr_trace;
	dfunction_t		*m_pr_xfunction;
	int				m_pr_xstatement;

	int				m_pr_argc;


	dfunction_t		*m_functions;
	char			*m_strings;
	ddef_t			*m_globaldefs;
	ddef_t			*m_fielddefs;
	dstatement_t	*m_statements;
	q1_val_t		*m_globals;

	int				m_edict_size;	// in bytes
	int				m_num_edicts;
	int				m_max_edicts;
	edict_t			**m_edicts;

	char		*m_lightstyles[MAX_LIGHTSTYLES];
private:

	typedef void(q1Progs::*builtin)();
	builtin m_builtins[79];
	int m_numbuiltins;
};

inline q1Progs::q1Progs(Server &sv):m_sv(sv) {
	for (int i = 0; i < MAX_LIGHTSTYLES; i++) {
		m_lightstyles[i] = 0;
	}
	m_numbuiltins = sizeof(m_builtins) / sizeof(m_builtins[0]);
#if 1
	int i = 0;
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_makevectors,	// void(entity e)	makevectors 		= #1;
		m_builtins[i++] = &q1Progs::PF_setorigin,	// void(entity e, vector o) setorigin	= #2;
		m_builtins[i++] = &q1Progs::PF_setmodel,	// void(entity e, string m) setmodel	= #3;
		m_builtins[i++] = &q1Progs::PF_setsize,	// void(entity e, vector min, vector max) setsize = #4;
		m_builtins[i++] = &q1Progs::PF_Fixme,	// void(entity e, vector min, vector max) setabssize = #5;
		m_builtins[i++] = &q1Progs::PF_break,	// void() break						= #6;
		m_builtins[i++] = &q1Progs::PF_random,	// float() random						= #7;
		m_builtins[i++] = &q1Progs::PF_sound,	// void(entity e, float chan, string samp) sound = #8;
		m_builtins[i++] = &q1Progs::PF_normalize,	// vector(vector v) normalize			= #9;
		m_builtins[i++] = &q1Progs::PF_error,	// void(string e) error				= #10;
		m_builtins[i++] = &q1Progs::PF_objerror,	// void(string e) objerror				= #11;
		m_builtins[i++] = &q1Progs::PF_vlen,	// float(vector v) vlen				= #12;
		m_builtins[i++] = &q1Progs::PF_vectoyaw,	// float(vector v) vectoyaw		= #13;
		m_builtins[i++] = &q1Progs::PF_Spawn,	// entity() spawn						= #14;
		m_builtins[i++] = &q1Progs::PF_Remove,	// void(entity e) remove				= #15;
		m_builtins[i++] = &q1Progs::PF_traceline,	// float(vector v1, vector v2, float tryents) traceline = #16;
		m_builtins[i++] = &q1Progs::PF_checkclient,	// entity() clientlist					= #17;
		m_builtins[i++] = &q1Progs::PF_Find,	// entity(entity start, .string fld, string match) find = #18;
		m_builtins[i++] = &q1Progs::PF_precache_sound,	// void(string s) precache_sound		= #19;
		m_builtins[i++] = &q1Progs::PF_precache_model,	// void(string s) precache_model		= #20;
		m_builtins[i++] = &q1Progs::PF_stuffcmd,	// void(entity client, string s)stuffcmd = #21;
		m_builtins[i++] = &q1Progs::PF_findradius,	// entity(vector org, float rad) findradius = #22;
		m_builtins[i++] = &q1Progs::PF_bprint,	// void(string s) bprint				= #23;
		m_builtins[i++] = &q1Progs::PF_sprint,	// void(entity client, string s) sprint = #24;
		m_builtins[i++] = &q1Progs::PF_dprint,	// void(string s) dprint				= #25;
		m_builtins[i++] = &q1Progs::PF_ftos,	// void(string s) ftos				= #26;
		m_builtins[i++] = &q1Progs::PF_vtos,	// void(string s) vtos				= #27;
		m_builtins[i++] = &q1Progs::PF_coredump,
		m_builtins[i++] = &q1Progs::PF_traceon,
		m_builtins[i++] = &q1Progs::PF_traceoff,
		m_builtins[i++] = &q1Progs::PF_eprint,	// void(entity e) debug print an entire entity
		m_builtins[i++] = &q1Progs::PF_walkmove, // float(float yaw, float dist) walkmove
		m_builtins[i++] = &q1Progs::PF_Fixme, // float(float yaw, float dist) walkmove
		m_builtins[i++] = &q1Progs::PF_droptofloor,
		m_builtins[i++] = &q1Progs::PF_lightstyle,
		m_builtins[i++] = &q1Progs::PF_rint,
		m_builtins[i++] = &q1Progs::PF_floor,
		m_builtins[i++] = &q1Progs::PF_ceil,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_checkbottom,
		m_builtins[i++] = &q1Progs::PF_pointcontents,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_fabs,
		m_builtins[i++] = &q1Progs::PF_aim,
		m_builtins[i++] = &q1Progs::PF_cvar,
		m_builtins[i++] = &q1Progs::PF_localcmd,
		m_builtins[i++] = &q1Progs::PF_nextent,
		m_builtins[i++] = &q1Progs::PF_particle,
		m_builtins[i++] = &q1Progs::PF_changeyaw,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_vectoangles,

		m_builtins[i++] = &q1Progs::PF_WriteByte,
		m_builtins[i++] = &q1Progs::PF_WriteChar,
		m_builtins[i++] = &q1Progs::PF_WriteShort,
		m_builtins[i++] = &q1Progs::PF_WriteLong,
		m_builtins[i++] = &q1Progs::PF_WriteCoord,
		m_builtins[i++] = &q1Progs::PF_WriteAngle,
		m_builtins[i++] = &q1Progs::PF_WriteString,
		m_builtins[i++] = &q1Progs::PF_WriteEntity,

		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,
		m_builtins[i++] = &q1Progs::PF_Fixme,

		m_builtins[i++] = &q1Progs::PF_MoveToGoal,
		m_builtins[i++] = &q1Progs::PF_precache_file,
		m_builtins[i++] = &q1Progs::PF_makestatic,

		m_builtins[i++] = &q1Progs::PF_changelevel,
		m_builtins[i++] = &q1Progs::PF_Fixme,

		m_builtins[i++] = &q1Progs::PF_cvar_set,
		m_builtins[i++] = &q1Progs::PF_centerprint,

		m_builtins[i++] = &q1Progs::PF_ambientsound,

		m_builtins[i++] = &q1Progs::PF_precache_model,
		m_builtins[i++] = &q1Progs::PF_precache_sound,		// precache_sound2 is different only for qcc
		m_builtins[i++] = &q1Progs::PF_precache_file,

		m_builtins[i++] = &q1Progs::PF_setspawnparms;
#endif
}

#define	G_FLOAT(o) (m_globals[o]._float)
#define	G_INT(o) (m_globals[o]._int)
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o) (&m_globals[o]._float)
#define	G_STRING(o) (m_strings + *(string_t *)&m_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&m_globals[o])

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
#define	E_STRING(e,o) (m_strings + *(string_t *)&((float*)&e->v)[o])

#define	RETURN_EDICT(e) (((int *)m_globals)[OFS_RETURN] = edict_to_prog(e))
#define EDICT_NUM_PTR(e) ((int *)((byte *)(e) + m_edict_size))

#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)
