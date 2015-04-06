#include "q1Progs.h"
#include "FileSystem.h"
#include "memPool.h"
#include "Host.h"
#include <stddef.h>
#include <stdlib.h>

extern SYS sys;
extern unsigned char *q1_palette;
extern memPool pool;

static char *pr_opnames[] =
{
	"DONE",

	"MUL_F",
	"MUL_V",
	"MUL_FV",
	"MUL_VF",

	"DIV",

	"ADD_F",
	"ADD_V",

	"SUB_F",
	"SUB_V",

	"EQ_F",
	"EQ_V",
	"EQ_S",
	"EQ_E",
	"EQ_FNC",

	"NE_F",
	"NE_V",
	"NE_S",
	"NE_E",
	"NE_FNC",

	"LE",
	"GE",
	"LT",
	"GT",

	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",
	"INDIRECT",

	"ADDRESS",

	"STORE_F",
	"STORE_V",
	"STORE_S",
	"STORE_ENT",
	"STORE_FLD",
	"STORE_FNC",

	"STOREP_F",
	"STOREP_V",
	"STOREP_S",
	"STOREP_ENT",
	"STOREP_FLD",
	"STOREP_FNC",

	"RETURN",

	"NOT_F",
	"NOT_V",
	"NOT_S",
	"NOT_ENT",
	"NOT_FNC",

	"IF",
	"IFNOT",

	"CALL0",
	"CALL1",
	"CALL2",
	"CALL3",
	"CALL4",
	"CALL5",
	"CALL6",
	"CALL7",
	"CALL8",

	"STATE",

	"GOTO",

	"AND",
	"OR",

	"BITAND",
	"BITOR"
};


int q1Progs::load() {
	int start_pool_used = pool.used();
	sysFile *file = sys.fileSystem.open("progs.dat");
	if (file == 0) {
		return -1;
	}
	if (file->read(reinterpret_cast<char *>(&m_programs), 0, sizeof(m_programs)) == 0) {
		return -1;
	}

	m_functions = new(pool)dfunction_t[m_programs.numfunctions];
	if (file->read(reinterpret_cast<char *>(m_functions), m_programs.ofs_functions, sizeof(dfunction_t)*m_programs.numfunctions) == 0) {
		return -1;
	}

	m_strings = new(pool) char[m_programs.numstrings + 1];
	m_strings[m_programs.numstrings] = 0;
	if (file->read(m_strings, m_programs.ofs_strings, sizeof(char)*m_programs.numstrings) == 0) {
		return -1;
	}

	m_globaldefs = new(pool)ddef_t[m_programs.numglobaldefs];
	if (file->read(reinterpret_cast<char *>(m_globaldefs), m_programs.ofs_globaldefs, sizeof(ddef_t)*m_programs.numglobaldefs) == 0) {
		return -1;
	}

	m_fielddefs = new(pool)ddef_t[m_programs.numfielddefs];
	if (file->read(reinterpret_cast<char *>(m_fielddefs), m_programs.ofs_fielddefs, sizeof(ddef_t)*m_programs.numfielddefs) == 0) {
		return -1;
	}

	m_statements = new(pool)dstatement_t[m_programs.numstatements];
	if (file->read(reinterpret_cast<char *>(m_statements), m_programs.ofs_statements, sizeof(dstatement_t)*m_programs.numstatements) == 0) {
		return -1;
	}

	m_globals = new(pool)q1_val_t[m_programs.numglobals];
	if (file->read(reinterpret_cast<char *>(m_globals), m_programs.ofs_globals, sizeof(q1_val_t)*m_programs.numglobals) == 0) {
		return -1;
	}

	m_global_struct = (globalvars_t*)m_globals;

	file->close();

	m_num_edicts = 0;
	m_max_edicts = MAX_EDICTS;
	m_edicts = (edict_t**)new(pool) byte[m_max_edicts*sizeof(edict_t*)];
	m_edict_size = m_programs.entityfields * 4 + sizeof (edict_t)-sizeof(entvars_t);

	m_pr_depth = 0;
	m_localstack_used = 0;

	int end_pool_used = pool.used();
	host.printf("%s loaded using %dk.\n",file->name(), (end_pool_used - start_pool_used) / 1024);

	return 0;
}

void q1Progs::run_error(char *error, ...) {
	va_list		argptr;
	char		string[1024];

	va_start(argptr, error);
	vsprintf(string, error, argptr);
	va_end(argptr);

	//PR_PrintStatement(pr_statements + pr_xstatement);
	//PR_StackTrace();
	printf("%s\n", string);

	if (m_pr_depth <= 0)
		m_pr_depth = 1;		
	
	// dump the stack so host_error can shutdown functions

	//Host_Error("Program error");
}

int  q1Progs::function_enter(dfunction_t *f) {
	int		i, j, c, o;

	m_pr_stack[m_pr_depth].s = m_pr_xstatement;
	m_pr_stack[m_pr_depth].f = m_pr_xfunction;
	m_pr_depth++;
	if (m_pr_depth >= MAX_STACK_DEPTH)
		run_error("stack overflow");

	// save off any locals that the new function steps on
	c = f->locals;
	if (m_localstack_used + c > LOCALSTACK_SIZE)
		run_error("PR_ExecuteProgram: locals stack overflow\n");

	for (i = 0; i < c; i++)
		m_localstack[m_localstack_used + i] = m_globals[f->parm_start + i]._int;
	m_localstack_used += c;

	// copy parameters
	o = f->parm_start;
	for (i = 0; i<f->numparms; i++)
	{
		for (j = 0; j<f->parm_size[i]; j++)
		{
			m_globals[o]._int = m_globals[OFS_PARM0 + i * 3 + j]._int;
			o++;
		}
	}

	m_pr_xfunction = f;
	return f->first_statement - 1;	// offset the s++
}

int  q1Progs::function_leave() {
	int		i, c;

	if (m_pr_depth <= 0)
		run_error("prog stack underflow");

	// restore locals from the stack
	c = m_pr_xfunction->locals;
	m_localstack_used -= c;
	if (m_localstack_used < 0)
		run_error("PR_ExecuteProgram: locals stack underflow\n");

	for (i = 0; i < c; i++)
		m_globals[m_pr_xfunction->parm_start + i]._int = m_localstack[m_localstack_used + i];

	// up stack
	m_pr_depth--;
	m_pr_xfunction = m_pr_stack[m_pr_depth].f;
	return m_pr_stack[m_pr_depth].s;
}

eval_t * q1Progs::prog_to_eval(int prog)
{
	edict_t *e;
	eval_t *v;
	int n = prog / m_edict_size;
	int x = prog % m_edict_size;
	if (n < 0 || n >= m_num_edicts)
		run_error("PROG_TO_EVAL: bad pointer");

	e = m_edicts[n];
	v = (eval_t *)(((byte *)e) + x);
	return v;
}

edict_t * q1Progs::prog_to_edict(int prog)
{
	int n = prog / m_edict_size;
	if (n < 0 || n >= m_num_edicts)
		run_error("PROG_TO_EDICT: bad pointer");

	return m_edicts[n];
}

int q1Progs::NUM_FOR_EDICT(edict_t *e)
{
	int		b;

	b = *(int *)((byte *)e + m_edict_size);

	if (b < 0 || b >= m_num_edicts)
		run_error("NUM_FOR_EDICT: bad pointer");
	return b;
}

#define EDICT_NUM_PTR(e) ((int *)((byte *)(e) + m_edict_size))

int	q1Progs::edict_to_prog(edict_t *e)
{
	int prog;
	int n = *EDICT_NUM_PTR(e);
	if (n < 0 || n >= m_num_edicts)
		run_error("EDICT_TO_PROG: bad pointer");
	//#define	EDICT_TO_PROG(e) ((byte *)e - (byte *)sv.edicts)

	prog = n * m_edict_size;

	return prog;

}


void q1Progs::program_execute(func_t fnum) {
	eval_t	*a, *b, *c;
	int			s;
	dstatement_t	*st;
	dfunction_t	*f, *newf;
	int		runaway;
	int		i;
	edict_t	*ed;
	int		exitdepth;
	eval_t	*ptr;

	if (!fnum || fnum >= m_programs.numfunctions)
	{
		//if (m_global_struct->self)
		//	ED_Print(PROG_TO_EDICT(pr_global_struct->self));
		run_error("PR_ExecuteProgram: NULL function");
	}

	f = &m_functions[fnum];

	runaway = 100000;
	m_pr_trace = false;

	// make a stack frame
	exitdepth = m_pr_depth;

	s = function_enter(f);

	while (1)
	{
		s++;	// next statement

		st = &m_statements[s];
		a = (eval_t *)(&m_globals[st->a]);
		b = (eval_t *)(&m_globals[st->b]);
		c = (eval_t *)(&m_globals[st->c]);

		if (!--runaway)
			run_error("runaway loop error");

		m_pr_xfunction->profile++;
		m_pr_xstatement = s;

		//if (m_pr_trace)
		//	PR_PrintStatement(st);

		switch (st->op)
		{
		case OP_ADD_F:
			c->_float = a->_float + b->_float;
			break;
		case OP_ADD_V:
			c->vector[0] = a->vector[0] + b->vector[0];
			c->vector[1] = a->vector[1] + b->vector[1];
			c->vector[2] = a->vector[2] + b->vector[2];
			break;

		case OP_SUB_F:
			c->_float = a->_float - b->_float;
			break;
		case OP_SUB_V:
			c->vector[0] = a->vector[0] - b->vector[0];
			c->vector[1] = a->vector[1] - b->vector[1];
			c->vector[2] = a->vector[2] - b->vector[2];
			break;

		case OP_MUL_F:
			c->_float = a->_float * b->_float;
			break;
		case OP_MUL_V:
			c->_float = a->vector[0] * b->vector[0]
				+ a->vector[1] * b->vector[1]
				+ a->vector[2] * b->vector[2];
			break;
		case OP_MUL_FV:
			c->vector[0] = a->_float * b->vector[0];
			c->vector[1] = a->_float * b->vector[1];
			c->vector[2] = a->_float * b->vector[2];
			//Con_Printf("%f %f %f %f\n",(a->_float),(b->vector[0]),(b->vector[1]),(b->vector[2]));
			//Con_Printf("%f %f %f\n",(c->vector[0]),(c->vector[1]),(c->vector[2]));
			break;
		case OP_MUL_VF:
			c->vector[0] = b->_float * a->vector[0];
			c->vector[1] = b->_float * a->vector[1];
			c->vector[2] = b->_float * a->vector[2];
			//Con_Printf("%f %f %f %f\n",(b->_float),(a->vector[0]),(a->vector[1]),(a->vector[2]));
			//Con_Printf("%f %f %f\n",(c->vector[0]),(c->vector[1]),(c->vector[2]));
			//Con_Printf("%d %d %d %d\n",(int)(b->_float),(int)(c->vector[0]),(int)(c->vector[1]),(int)(c->vector[2]));
			break;

		case OP_DIV_F:
			c->_float = a->_float / b->_float;
			break;

		case OP_BITAND:
			c->_float = (float)((int)a->_float & (int)b->_float);
			break;

		case OP_BITOR:
			c->_float = (float)((int)a->_float | (int)b->_float);
			break;


		case OP_GE:
			c->_float = a->_float >= b->_float;
			break;
		case OP_LE:
			c->_float = a->_float <= b->_float;
			break;
		case OP_GT:
			c->_float = a->_float > b->_float;
			break;
		case OP_LT:
			c->_float = a->_float < b->_float;
			break;
		case OP_AND:
			c->_float = a->_float && b->_float;
			break;
		case OP_OR:
			c->_float = a->_float || b->_float;
			break;

		case OP_NOT_F:
			c->_float = !a->_float;
			break;
		case OP_NOT_V:
			c->_float = !a->vector[0] && !a->vector[1] && !a->vector[2];
			break;
		case OP_NOT_S:
			c->_float = !a->string || !m_strings[a->string];
			break;
		case OP_NOT_FNC:
			c->_float = !a->function;
			break;
		case OP_NOT_ENT:
			c->_float = (prog_to_edict(a->edict) == m_edicts[0]);
			break;

		case OP_EQ_F:
			c->_float = a->_float == b->_float;
			break;
		case OP_EQ_V:
			c->_float = (a->vector[0] == b->vector[0]) &&
				(a->vector[1] == b->vector[1]) &&
				(a->vector[2] == b->vector[2]);
			break;
		case OP_EQ_S:
			c->_float = !strcmp(m_strings + a->string, m_strings + b->string);
			break;
		case OP_EQ_E:
			c->_float = a->_int == b->_int;
			break;
		case OP_EQ_FNC:
			c->_float = a->function == b->function;
			break;


		case OP_NE_F:
			c->_float = a->_float != b->_float;
			break;
		case OP_NE_V:
			c->_float = (a->vector[0] != b->vector[0]) ||
				(a->vector[1] != b->vector[1]) ||
				(a->vector[2] != b->vector[2]);
			break;
		case OP_NE_S:
			c->_float = (float)strcmp(m_strings + a->string, m_strings + b->string);
			break;
		case OP_NE_E:
			c->_float = a->_int != b->_int;
			break;
		case OP_NE_FNC:
			c->_float = a->function != b->function;
			break;

			//==================
		case OP_STORE_F:
		case OP_STORE_ENT:
		case OP_STORE_FLD:		// integers
		case OP_STORE_S:
		case OP_STORE_FNC:		// pointers
			b->_int = a->_int;
			break;
		case OP_STORE_V:
			b->vector[0] = a->vector[0];
			b->vector[1] = a->vector[1];
			b->vector[2] = a->vector[2];
			break;

		case OP_STOREP_F:
		case OP_STOREP_ENT:
		case OP_STOREP_FLD:		// integers
		case OP_STOREP_S:
		case OP_STOREP_FNC:		// pointers
			//FIXME FIXME FIXME
			//ptr = (eval_t *)((byte *)sv.edicts + b->_int);
			ptr = prog_to_eval(b->_int);
			ptr->_int = a->_int;
			//FIXME FIXME FIXME
			break;
		case OP_STOREP_V:
			//FIXME FIXME FIXME
			//ptr = (eval_t *)((byte *)sv.edicts + b->_int);
			ptr = prog_to_eval(b->_int);
			ptr->vector[0] = a->vector[0];
			ptr->vector[1] = a->vector[1];
			ptr->vector[2] = a->vector[2];
			//FIXME FIXME FIXME
			break;

		case OP_ADDRESS:
			ed = prog_to_edict(a->edict);

			//FIXME FIXME FIXME
			if (ed == (edict_t *)m_edicts[0] && host.server_state() == ss_active)
				run_error("assignment to world entity");
			//c->_int = (byte *)((int *)&ed->v + b->_int) - (byte *)sv.edicts;
			c->_int = edict_to_prog(ed) + 4 * b->_int + offsetof(edict_t, v);
			//FIXME FIXME FIXME
			break;

		case OP_LOAD_F:
		case OP_LOAD_FLD:
		case OP_LOAD_ENT:
		case OP_LOAD_S:
		case OP_LOAD_FNC:
			ed = prog_to_edict(a->edict);

			a = (eval_t *)((int *)&ed->v + b->_int);
			c->_int = a->_int;
			break;

		case OP_LOAD_V:
			ed = prog_to_edict(a->edict);

			a = (eval_t *)((int *)&ed->v + b->_int);
			c->vector[0] = a->vector[0];
			c->vector[1] = a->vector[1];
			c->vector[2] = a->vector[2];
			break;

			//==================

		case OP_IFNOT:
			if (!a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_IF:
			if (a->_int)
				s += st->b - 1;	// offset the s++
			break;

		case OP_GOTO:
			s += st->a - 1;	// offset the s++
			break;

		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
			m_pr_argc = st->op - OP_CALL0;
			if (!a->function)
				run_error("NULL function");

			newf = &m_functions[a->function];

			if (newf->first_statement < 0)
			{	// negative statements are built in functions
				i = -newf->first_statement;
				if (i >= m_numbuiltins)
					run_error("Bad builtin call number");
				(this->*m_builtins[i])();
				break;
			}

			s = function_enter(newf);
			break;

		case OP_DONE:
		case OP_RETURN:
			m_globals[OFS_RETURN] = m_globals[st->a];
			m_globals[OFS_RETURN + 1] = m_globals[st->a + 1];
			m_globals[OFS_RETURN + 2] = m_globals[st->a + 2];

			s = function_leave();
			if (m_pr_depth == exitdepth)
				return;		// all done
			break;

		case OP_STATE:
			ed = prog_to_edict(m_global_struct->self);
			ed->v.nextthink = m_global_struct->time + 0.1f;

			if (a->_float != ed->v.frame)
			{
				ed->v.frame = a->_float;
			}
			ed->v.think = b->function;
			break;

		default:
			run_error("Bad opcode %i", st->op);
		}
	}

}

/*
============
ED_FindField
============
*/
ddef_t *q1Progs::ED_FindField(char *name) {
	ddef_t		*def;
	int			i;

	for (i = 0; i<m_programs.numfielddefs; i++) {
		def = &m_fielddefs[i];
		if (!strcmp(m_strings + def->s_name, name))
			return def;
	}
	return 0;
}

/*
============
ED_FindFunction
============
*/
dfunction_t *q1Progs::ED_FindFunction(char *name) {
	dfunction_t		*func;
	int				i;

	for (i = 0; i<m_programs.numfunctions; i++) {
		func = &m_functions[i];
		if (!strcmp(m_strings + func->s_name, name))
			return func;
	}
	return 0;
}

char *q1Progs::ED_NewString(char *string)
{
	char	*new1, *new_p;
	int		i, l;

	l = strlen(string) + 1;
	new1 = new(pool) char[l];
	new_p = new1;

	for (i = 0; i< l; i++)
	{
		if (string[i] == '\\' && i < l - 1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return new1;
}

/*
=============
ED_ParseEval

Can parse either fields or globals
returns false if error
=============
*/
bool q1Progs::ED_ParseEpair(void *base, ddef_t *key, char *s) {
	int		i;
	char	string[128];
	ddef_t	*def;
	char	*v, *w;
	void	*d;
	dfunction_t	*func;

	d = (void *)((int *)base + key->ofs);

	switch (key->type & ~DEF_SAVEGLOBAL)
	{
	case ev_string:
		*(string_t *)d = ED_NewString(s) - m_strings;
		break;

	case ev_float:
		*(float *)d = atof(s);
		break;

	case ev_vector:
		strcpy(string, s);
		v = string;
		w = string;
		for (i = 0; i<3; i++)
		{
			while (*v && *v != ' ')
				v++;
			*v = 0;
			((float *)d)[i] = atof(w);
			w = v = v + 1;
		}
		break;

	case ev_entity:
		*(int *)d = edict_to_prog(EDICT_NUM2(atoi(s)));
		break;

	case ev_field:
		def = ED_FindField(s);
		if (!def)
		{
			printf("Can't find field %s\n", s);
			return false;
		}
		*(int *)d = G_INT(def->ofs);
		break;

	case ev_function:
		func = ED_FindFunction(s);
		if (!func)
		{
			printf("Can't find function %s\n", s);
			return false;
		}
		*(func_t *)d = func - m_functions;
		break;

	default:
		break;
	}
	return true;
}

#define	MAX_FIELD_LEN	64
#define GEFV_CACHESIZE	2

typedef struct {
	ddef_t	*pcache;
	char	field[MAX_FIELD_LEN];
} gefv_cache;

static gefv_cache	gefvCache[GEFV_CACHESIZE] = { { NULL, "" }, { NULL, "" } };

eval_t * q1Progs::GetEdictFieldValue(edict_t *ed, char *field)
{
	ddef_t			*def = NULL;
	int				i;
	static int		rep = 0;

	for (i = 0; i<GEFV_CACHESIZE; i++)
	{
		if (!strcmp(field, gefvCache[i].field))
		{
			def = gefvCache[i].pcache;
			goto Done;
		}
	}

	def = ED_FindField(field);

	if (strlen(field) < MAX_FIELD_LEN)
	{
		gefvCache[rep].pcache = def;
		strcpy(gefvCache[rep].field, field);
		rep ^= 1;
	}

Done:
	if (!def)
		return NULL;

	return (eval_t *)((char *)&ed->v + def->ofs * 4);
}

ddef_t *q1Progs::ED_FieldAtOfs(int ofs)
{
	ddef_t		*def;
	int			i;

	for (i = 0; i<m_programs.numfielddefs; i++)
	{
		def = &m_fielddefs[i];
		if (def->ofs == ofs)
			return def;
	}
	return 0;
}

ddef_t *q1Progs::ED_FindGlobal(char *name) {
	ddef_t		*def;
	int			i;

	for (i = 0; i<m_programs.numglobaldefs; i++)
	{
		def = &m_globaldefs[i];
		if (!strcmp(m_strings + def->s_name, name))
			return def;
	}
	return 0;
}

char *q1Progs::valueString(int type, eval_t *val) {
	static char	line[256];
	ddef_t		*def;
	dfunction_t	*f;
	
	type &= ~DEF_SAVEGLOBAL;

	switch (type)
	{
	case ev_string:
		sprintf(line, "%s", m_strings + val->string);
		break;
	case ev_entity:
		sprintf(line, "%i", NUM_FOR_EDICT(prog_to_edict(val->edict)));
		break;
	case ev_function:
		f = m_functions + val->function;
		sprintf(line, "%s", m_strings + f->s_name);
		break;
	case ev_field:
		def = ED_FieldAtOfs(val->_int);
		sprintf(line, "%s", m_strings + def->s_name);
		break;
	case ev_void:
		sprintf(line, "void");
		break;
	case ev_float:
		sprintf(line, "%f", val->_float);
		break;
	case ev_vector:
		sprintf(line, "%f %f %f", val->vector[0], val->vector[1], val->vector[2]);
		break;
	default:
		sprintf(line, "bad type %i", type);
		break;
	}

	return line;
}

void q1Progs::ED_WriteGlobals(FILE *f)
{
	ddef_t		*def;
	int			i;
	char		*name;
	int			type;
	int len;

	fprintf(f, "{\n");
	for (i = 0; i<m_programs.numglobaldefs; i++)
	{
		def = &m_globaldefs[i];
		type = def->type;
		if (!(def->type & DEF_SAVEGLOBAL))
			continue;
		type &= ~DEF_SAVEGLOBAL;

		if (type != ev_string
			&& type != ev_float
			&& type != ev_entity)
			continue;

		name = m_strings + def->s_name;
		fprintf(f, "\"%s\" ", name);
		fprintf(f, "\"%s\"\n", valueString(type, (eval_t *)&m_globals[def->ofs]));
	}
	fprintf(f, "}\n");
}

static int type_size[8] = { 1, sizeof(string_t) / 4, 1, 3, 1, 1, sizeof(func_t) / 4, sizeof(void *) / 4 };
void q1Progs::ED_Write(FILE *f, edict_t *ed)
{
	ddef_t	*d;
	int		*v;
	int		i, j;
	char	*name;
	int		type;

	fprintf(f, "{\n");

	if (ed->free)
	{
		fprintf(f, "}\n");
		return;
	}

	for (i = 1; i<m_programs.numfielddefs; i++)
	{
		d = &m_fielddefs[i];
		name = m_strings + d->s_name;
		if (name[strlen(name) - 2] == '_')
			continue;	// skip _x, _y, _z vars

		v = (int *)((char *)&ed->v + d->ofs * 4);

		// if the value is still all 0, skip the field
		type = d->type & ~DEF_SAVEGLOBAL;
		for (j = 0; j<type_size[type]; j++)
			if (v[j])
				break;
		if (j == type_size[type])
			continue;

		fprintf(f, "\"%s\" ", name);
		fprintf(f, "\"%s\"\n", valueString(d->type, (eval_t *)v));
	}

	fprintf(f, "}\n");
}

void q1Progs::ED_Write(FILE *f) {
	for (int i = 0; i < m_num_edicts; i++) {
		ED_Write(f, EDICT_NUM(i));
	}
}