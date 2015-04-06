#include "FileSystem.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int sysFileDir::Load(char *filename) {
	FILE *fp = 0;
	char header[64];
	int length = 0;
	struct stat fstat;

	if(stat(filename,&fstat)) {
		return -1;
	}

	if ((fstat.st_mode & _IFDIR) != 0) {
		m_isdir = true;
		int len = strlen(filename)+1;
		//add extra for trailing / if needed
		m_path = new char[len+1];
		memcpy(m_path,filename,len);
		if(m_path[len-2] != '\\' && m_path[len-2] != '/') {
			m_path[len-1] = '\\';
			m_path[len] = 0;
		}
		return 0;
	}

	if(!(fp = fopen(filename,"rb"))) {
		return -1;
	}
	fseek(fp,0,SEEK_END);
	length = ftell(fp);
	if(length <= 0) {
		fclose(fp);
		return -1;
	}

	fseek(fp,0,SEEK_SET);
	fread(header,sizeof(header),1,fp);

	if(strncmp("WAD2",header,4)==0) {
		parse_wad2(fp, header,length);
	} else if(strncmp("IWAD",header,4)==0) {
		parse_pwad(fp, header,length);
	} else if(strncmp("PACK",header,4)==0) {
		parse_pack(fp, header,length);
	}

	fclose(fp);

	if(m_count > 0) {
		m_data = new void*[m_count];
		memset(m_data,0,m_count * sizeof(void *));
	}

	int len = strlen(filename)+1;
	m_path = new char[len];
	memcpy(m_path,filename,len);

	printf("%s: %d files\n", m_path, m_count);

	return 0;
}

int sysFileDir::LoadFromMemory(char *header,int length) {

	if(strncmp("WAD2",header,4)==0) {
		parse_wad2(0, header,length);
	} else if(strncmp("IWAD",header,4)==0) {
		parse_pwad(0, header,length);
	} else if(strncmp("PACK",header,4)==0) {
		parse_pack(0, header,length);
	}

	if(m_count > 0) {
		m_data = new void*[m_count];
		memset(m_data,0,m_count * sizeof(void *));

		for(int i = 0;i< m_count;i++) {
			m_data[i] = &header[m_files[i].ofs];
		}
	}

	m_path = new char[1];
	*m_path = 0;

	return 0;
}

void* sysFileDir::Get(int idx) {
	if(idx < 0 || idx > m_count) {
		return 0;
	}
	if(m_data[idx]) {
		return m_data[idx];
	}
	int len = m_files[idx].len;
	int ofs = m_files[idx].ofs;
	FILE *fp;
	if(!(fp = fopen(m_path,"rb"))) {
		return 0;
	}
	char *buffer = new char[len];
	fseek(fp,ofs,SEEK_SET);
	fread(buffer,len,1,fp);
	fclose(fp);
	m_data[idx] = buffer;

	return buffer;
}

int sysFileDir::Find(char *name) {
	for (int i = 0; i<m_count; i++) {
		if (!strncasecmp(m_files[i].name, name, m_max_filename_length)) {
			return i;
		}
	}

	return -1;
}

int instr(char *src, int src_len, char *sub) {
	int sub_len = strlen(sub);
	int srch_len = src_len - sub_len;

	for (int pos = 0; pos < srch_len; pos++) {
		if (!strncasecmp(sub, &src[pos], sub_len)) {
			return pos;
		}
	}
	
	return -1;
}
int sysFileDir::dir(char *partial, int startnum, int maxnum, char **list) {
	int num = startnum;
	for (int i = 0; i<m_count; i++) {
		if (instr(m_files[i].name, m_max_filename_length, partial) != -1) {
			if (num >= maxnum) {
				break;
			}
			list[num++] = m_files[i].name;
		}
	}

	return num;
}

FileSystemStat sysFileDir::operator[](char *name) {
	FileSystemStat stat;

	int idx = Find(name);
	if(idx < 0) {
		stat.idx = 0;
		stat.fsp = 0;
		stat.len = 0;
		stat.ofs = 0;
	} else {
		stat.idx = idx;
		stat.fsp = this;
		stat.len = m_files[idx].len;
		stat.ofs = m_files[idx].ofs;
	}
	return stat;
}

FileSystemStat sysFileDir::operator[]( int idx ) {
	FileSystemStat stat;

	if(idx < 0 || idx >= m_count) {
		stat.idx = 0;
		stat.fsp = 0;
		stat.len = 0;
		stat.ofs = 0;
	} else {
		stat.idx = idx;
		stat.fsp = this;
		stat.len = m_files[idx].len;
		stat.ofs = m_files[idx].ofs;
	}
	return stat;
}

sysFile* sysFileDir::open(char *name) {
	if(m_isdir) {
		char fullname[1024];
		int len1 = strlen(m_path);
		int len2 = strlen(name);
		if( (len1 + len2) >= 1024) {
			return 0;
		}
		struct stat st;
		memcpy(fullname,m_path,len1);
		memcpy(&fullname[len1],name,len2+1);
		if(stat(fullname,&st)) {
			return 0;
		}
		return new sysFile(fullname,0,st.st_size);
	}

	int idx = Find(name);
	if(idx < 0 || idx >= m_count) {
		return 0;
	}
	return new sysFile(m_path,m_files[idx].ofs,m_files[idx].len);
}

typedef struct
{
	char		identification[4];		// should be WAD2 or 2DAW
	int			numlumps;
	int			infotableofs;
} wadinfo_t;

typedef struct
{
	int			filepos;
	int			size;					// uncompressed
	char		name[8];				// must be null terminated
} pwadLump_t;

int sysFileDir::parse_pwad(FILE *fp, char *header, int length) {
	wadinfo_t *wad = (wadinfo_t *)header;
	int num_lumps = wad->numlumps;
	m_files = new FileSystemFile[num_lumps];
	int lumps_length = num_lumps*sizeof(pwadLump_t);
	pwadLump_t *lumps = 0;
	char *names = new char[num_lumps*8];

	memset(m_files,0,num_lumps*sizeof(FileSystemFile));

	//if the file is null then the whole file follows the header
	if(fp) {
		lumps = new pwadLump_t[num_lumps];
		fseek(fp,wad->infotableofs,SEEK_SET);
		fread(lumps,lumps_length,1,fp);
	} else {
		lumps = (pwadLump_t *)&header[wad->infotableofs];
	}

	for(int i=0;i<num_lumps;i++,names+=8) {
		memcpy(names,lumps[i].name,8);
		m_files[i].name = names;
		m_files[i].ofs = lumps[i].filepos;
		m_files[i].len = lumps[i].size;
	}

	if(fp && lumps) {
		delete [] lumps;
	}

	m_max_filename_length = 8;
	m_count = num_lumps;
	return 0;
}

typedef struct
{
	int			filepos;
	int			disksize;
	int			size;					// uncompressed
	char		type;
	char		compression;
	char		pad1, pad2;
	char		name[16];				// must be null terminated
} wad2Lump_t;

int sysFileDir::parse_wad2(FILE *fp, char *header, int length) {
	wadinfo_t *wad = (wadinfo_t *)header;
	int num_lumps = wad->numlumps;
	m_files = new FileSystemFile[num_lumps];
	int lumps_length = num_lumps*sizeof(wad2Lump_t);
	wad2Lump_t *lumps = 0;
	char *names = new char[num_lumps*16];

	memset(m_files,0,num_lumps*sizeof(FileSystemFile));

	//if the file is null then the whole file follows the header
	if(fp) {
		lumps = new wad2Lump_t[num_lumps];
		fseek(fp,wad->infotableofs,SEEK_SET);
		fread(lumps,lumps_length,1,fp);
	} else {
		lumps = (wad2Lump_t *)&header[wad->infotableofs];
	}

	for(int i=0;i<num_lumps;i++,names+=16) {
		memcpy(names,lumps[i].name,16);
		m_files[i].name = names;
		m_files[i].ofs = lumps[i].filepos;
		m_files[i].len = lumps[i].size;
	}

	if(fp && lumps) {
		delete [] lumps;
	}

	m_max_filename_length = 16;
	m_count = num_lumps;
	return 0;
}

// pack file on disk
typedef struct
{
	char    name[56];
	int             filepos, filelen;
} dpackfile_t;

typedef struct
{
	char    id[4];
	int             dirofs;
	int             dirlen;
} dpackheader_t;

int sysFileDir::parse_pack(FILE *fp, char *header, int length) {
	dpackheader_t *pack = (dpackheader_t *)header;
	int num_lumps = pack->dirlen/sizeof(dpackfile_t);
	m_files = new FileSystemFile[num_lumps];
	int lumps_length = num_lumps*sizeof(dpackfile_t);
	dpackfile_t *lumps = 0;
	char *names = new char[num_lumps*56];

	memset(m_files,0,num_lumps*sizeof(FileSystemFile));

	//if the file is null then the whole file follows the header
	if(fp) {
		lumps = new dpackfile_t[num_lumps];
		fseek(fp,pack->dirofs,SEEK_SET);
		fread(lumps,lumps_length,1,fp);
	} else {
		lumps = (dpackfile_t *)&header[pack->dirofs];
	}

	for(int i=0;i<num_lumps;i++,names+=56) {
		memcpy(names,lumps[i].name,56);
		m_files[i].name = names;
		m_files[i].ofs = lumps[i].filepos;
		m_files[i].len = lumps[i].filelen;
		//printf("%d: %s\n", i, names);
	}

	if(fp && lumps) {
		delete [] lumps;
	}

	m_max_filename_length = 56;
	m_count = num_lumps;
	return 0;
}

int FileSystem::grow() {
	//expand the list if needed
	if( (m_paths_max-m_paths_cnt) == 0) {
		sysFileDir **paths = new sysFileDir*[m_paths_max+4];
		if(paths == 0) {
			return -1;
		}
		m_paths_max += 4;
		memcpy(paths,m_paths,m_paths_cnt*sizeof(sysFileDir*));
		m_paths = paths;
	}

	return 0;
}

sysFileDir* FileSystem::Add(char *path) {
	//expand the list if needed
	if(grow() < 0) {
		return 0;
	}
	//add the new path
	sysFileDir *fpath = new sysFileDir();
	if (fpath->Load(path) == -1) {
		delete fpath;
		return 0;
	}
	m_paths[m_paths_cnt++] = fpath;

	return fpath;
}

sysFileDir* FileSystem::Add(char *buffer,int length) {
	//expand the list if needed
	if(grow() < 0) {
		return 0;
	}
	//add the new path
	sysFileDir *fpath = new sysFileDir();
	fpath->LoadFromMemory(buffer,length);
	m_paths[m_paths_cnt++] = fpath;

	return fpath;
}

FileSystemStat FileSystem::operator[]( char *name ) {
	FileSystemStat stat;

	int cnt = m_paths_cnt;
	while(cnt-- > 0) {
		sysFileDir *path = m_paths[cnt];
		int idx = path->Find(name);
		if(idx != -1) {
			return (*path)[idx];
		}
	}

	stat.idx = 0;
	stat.fsp = 0;
	stat.len = 0;
	stat.ofs = 0;
	return stat;
}

int FileSystem::dir(char*partial, int maxnum, char **list) {
	int cnt = m_paths_cnt;
	int found = 0;
	while (cnt-- > 0) {
		sysFileDir *path = m_paths[cnt];
		int cnt = path->dir(partial,found,maxnum,list);
		found = cnt;
	}
	return found;
}

sysFile* FileSystem::open(char *name) {

	int cnt = m_paths_cnt;
	while (cnt-- > 0) {
		sysFileDir *path = m_paths[cnt];
		sysFile* file = path->open(name);
		if (file) {
			return file;
		}
	}

	return 0;
}

FILE* FileSystem::create(char *name) {
	char base[1024];
	int cnt = m_paths_cnt;
	//default to current directory
	*base = 0;
	//try to find a directory (rather than a pak or wad)
	while (cnt-- > 0) {
		sysFileDir *path = m_paths[cnt];
		if (path->is_dir()) {
			path->name(base);
			break;
		}
	}
	strcat(base, name);
	return fopen(base, "wb");
}

void* FileSystem::Get(FileSystemStat &stat) {
	if(stat.fsp == 0) {
		return 0;
	}
	return stat.fsp->Get(stat.idx);
}