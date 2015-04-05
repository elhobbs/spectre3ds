#pragma once
#include <stdio.h>
#include <string.h>

using namespace std;

typedef unsigned char u8;

typedef struct {
	char	*name;
	int		len;
	int		ofs;
} FileSystemFile;

class sysFileDir;

typedef struct {
	int		idx;
	int		len;
	int		ofs;
	sysFileDir *fsp;
} FileSystemStat;

class sysFile {
public:
	sysFile(char *name,int ofs, int len);
	~sysFile();
	int length();
	int read(char *buffer,int ofs,int len);
	int close();
	bool isopen();
	char *name();
private:

	int		m_ofs;
	int		m_pos;
	int		m_len;
	char	m_name[260];
	FILE*	m_file;
};

inline sysFile::sysFile(char *name,int ofs, int len) {
	m_ofs = ofs;
	m_len = 0;
	m_pos = 0;
	m_file = 0;
	strcpy(m_name, name);
	m_file = fopen( name, "rb");
	if(m_file) {
		//calculate the length if needed
		if(len < 0) {
			fseek(m_file,m_ofs,SEEK_END);
			m_len = ftell(m_file);
		} else {
			m_len = len;
		}
		fseek(m_file,m_ofs,SEEK_SET);
	}
}

inline sysFile::~sysFile() {
	close();
}

inline bool sysFile::isopen() {
	return m_file ? true : false;
}

inline int sysFile::length() {
	return m_len;
}

inline char* sysFile::name() {
	return m_name;
}

inline int sysFile::close() {
	if (m_file) {
		fclose(m_file);
		m_file = 0;
		m_pos = 0;
	}
	return 0;
}

inline int sysFile::read(char *buffer,int ofs, int len) {
	bool needs_close = false;
	//verify still open
	if (!isopen()) {
		m_file = fopen(m_name, "rb");
		if (!isopen()) {
			return 0;
		}
		needs_close = true;
	}
	//seek to the right pos if needed
	if(ofs >= 0 && ofs != m_pos) {
		fseek(m_file,m_ofs+ofs,SEEK_SET);
		m_pos = ofs;
	}
	//dont read past the end of the file
	if( (len + m_pos) > m_len ) {
		len = m_len - m_pos;
	}
	//read into the buffer
	int cb_read = fread(buffer,1,len,m_file);
	m_pos += cb_read;

	if (needs_close) {
		close();
	}
	return cb_read;
}


class sysFileDir {
public:
	sysFileDir();
	int		Load(char *path);
	int		LoadFromMemory(char *buffer,int length);
	int		Find(char *name);
	void*	Get(int idx);
	void	FreeData(int idx);
	int		dir(char *partial, int startnum, int maxnum, char **list);

	sysFile*	open(char *name);

	FileSystemStat operator[]( char *name );
	FileSystemStat operator[]( int idx );

private:
	int parse_wad2(FILE *fp, char *header, int length);
	int parse_pwad(FILE *fp, char *header,int length);
	int parse_pack(FILE *fp, char *header,int length);

	bool			m_isdir;
	int				m_count;
	FileSystemFile*	m_files;
	int				m_max_filename_length;
	void**			m_data;
	char*			m_path;
};

inline sysFileDir::sysFileDir( void ) {
	m_isdir = false;
	m_files = 0;
	m_count = 0;
	m_data = 0;
	m_max_filename_length = 0;
}

class FileSystem {
public:
	FileSystem();

	sysFileDir* Add(char *path);
	sysFileDir* Add(char *buffer, int length);
	FileSystemStat operator[]( char *name );
	void* Get(FileSystemStat &stat);

	sysFile* open(char *name);

	int dir(char*partial, int maxnum, char **list);

private:
	int grow();
	int m_paths_max;
	int m_paths_cnt;
	sysFileDir **m_paths;
};

inline FileSystem::FileSystem() {
	m_paths_max = 8;
	m_paths_cnt = 0;
	m_paths = new sysFileDir*[m_paths_max];
}
