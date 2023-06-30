#include <iostream>
#include <list>
#include <vector>
#include "d3dx9math.h"

#include <sstream>
#include <deque>
#include <iostream>

#include "tinyxml.h"

using namespace std;

char *out_file_data=NULL;


template< class Type >
string ToString( Type value ) {
   stringstream s;
   s << value;
   return s.str();
}

FILE *public_log=NULL;
unsigned int class_name_global_address=0;


void lprintf(char *msg, ...) {
    va_list fmtargs;
    char buffer[10000];

    va_start(fmtargs,msg);
    vsnprintf(buffer,sizeof(buffer)-1,msg,fmtargs);
    va_end(fmtargs);
    //printf("%s",buffer);

    fprintf(public_log, buffer);
    fflush(public_log);
}


void char_endian_swap(int i, int sz) {
    //lprintf("Requested char endian swap %d %d\n", i, sz);

    char t=0;
    if (sz%2) return;

    for (int c=0; c<sz/2; c++) {
        t = out_file_data[i+c];
        out_file_data[i+c] = out_file_data[i+(sz-c)-1];
        out_file_data[i+(sz-c)-1] = t;
    }
}


string getTypeString(int c) {
    if (c == 0) return "void";              // Done
    if (c == 1) return "bool";              // Done
    if (c == 2) return "char";              // Done
    if (c == 3) return "int8";              // Done
    if (c == 4) return "uint8";             // Done
    if (c == 5) return "int16";             // Done
    if (c == 6) return "uint16";            // Done
    if (c == 7) return "int32";             // Done
    if (c == 8) return "uint32";            // Done
    if (c == 9) return "int64";             // Done
    if (c == 10) return "uint64";           // Done
    if (c == 11) return "real";             // Done
    if (c == 12) return "vector4";          // Done
    if (c == 13) return "quaternion";       // Done
    if (c == 14) return "matrix3";          // Done
    if (c == 15) return "rotation";          // Done
    if (c == 16) return "qstransform";      // Done
    if (c == 17) return "matrix4";          // Done
    if (c == 18) return "transform";        // Done
    if (c == 19) return "zero";             // Deprecated
    if (c == 20) return "pointer";          // Done
    if (c == 21) return "function_pointer"; // Done
    if (c == 22) return "array";            // Done?
    if (c == 23) return "inplace_array";     // Done?
    if (c == 24) return "enum";             // Done
    if (c == 25) return "struct";
    if (c == 26) return "simple_array";      // Simple array (ptr(typed) and size only)
    if (c == 27) return "homogeneous_array";      // Simple array of homogeneous types, so is a class id followed by a void* ptr and size
    if (c == 28) return "variant";           // hkVariant (void* and hkClass*) type
    if (c == 29) return "cstring";          //
    if (c == 30) return "ulong";            // Done
    if (c == 31) return "flags";            // hkFlags<ENUM,STORAGE> - 8,16,32 bits of named values.
    if (c == 32) return "half";             // Done
    if (c == 33) return "string_pointer";   // Done


    return "unknown("+ToString(c)+")";
}


inline void int32_endian_swap(int& x) {
    x = (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

inline void uint16_endian_swap(unsigned short& x) {
    x = (x>>8) | (x<<8);
}

inline void uint32_endian_swap(unsigned int& x) {
    x = (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}


void fixFileAlign(FILE *fp) {
    unsigned int add=16-(ftell(fp)%16);
    if (add == 16) add = 0;
    fseek(fp, add, SEEK_CUR);
}


class hkClassName {
    public:
        unsigned int tag;
        string name;

        hkClassName(){}
};

list<hkClassName> hkClassNames;


class hkTypeMember {
    public:
        unsigned char tag[2];
        unsigned short array_size;
        unsigned short struct_type;
        unsigned short offset;
        unsigned int structure_address;
        string name;
        string structure;

        hkTypeMember() {}
};

class hkEnum {
    public:
        unsigned int id;
        string name;
};

class hkType {
    public:
        unsigned int object_size;
        string name;
        string class_name;
        unsigned int described_version;
        unsigned int num_implemented_interfaces;
        unsigned int declared_enums;
        unsigned int address;

        vector<hkTypeMember> members;
        vector<hkEnum> enums;
        vector< vector<hkEnum> > sub_enums;
        vector<string> sub_enum_names;

        hkType *parent;
        unsigned int parent_address;

        hkType() {}
        void reset() {
            members.clear();
            enums.clear();
            sub_enums.clear();
            sub_enum_names.clear();
            name="";

            parent=NULL;
            parent_address=0;
        }
};

list<hkType> hkTypes;


class hkLink {
    public:
        unsigned int type;
        unsigned int address_1;
        unsigned int address_2;
        hkType *type_parent;
        hkType *type_node;
};

list<hkLink> type_links;
list<hkLink> data_links;


class hkPointer {
    public:
        unsigned abs_address;
        unsigned target_address;
};

list<hkPointer> data_pointers;
list<hkPointer> data_global_pointers;


class hkPackfileHeader_conv {
	//+version(1)
	public:
			/// Default constructor
		hkPackfileHeader_conv()
		{
			m_magic[0] = 0x57e0e057; // both endian agnostic (byte symmetric)
			m_magic[1] = 0x10c0c010;
			m_contentsVersion[0] = 0;
			m_flags = 0; // Set to 1 when a packfile is loaded in-place to signify that the packfile is loaded
		}

	public:

			/// Magic file identifier. See constructor for values.
		int m_magic[2];

			/// This is a user settable tag.
		int  m_userTag;

			/// Binary file version. Currently 9
		int  m_fileVersion;

			/// The structure layout rules used by this file.
		unsigned char m_layoutRules[4];

			/// Number of packfilesections following this header.
		int m_numSections;

			/// Where the content's data structure is (section and offset within that section).
		int m_contentsSectionIndex;
		int m_contentsSectionOffset;

			/// Where the content's class name is (section and offset within that section).
		int m_contentsClassNameSectionIndex;
		int m_contentsClassNameSectionOffset;

			/// Future expansion
		char m_contentsVersion[16];

			///
		int m_flags;
		int m_pad[1];

		void convertToPC() {
		    m_userTag=0;
		    int32_endian_swap(m_fileVersion);
		    m_layoutRules[0] = 4;
		    m_layoutRules[1] = 1;
		    m_layoutRules[2] = 0;
		    m_layoutRules[3] = 1;

		    int32_endian_swap(m_numSections);
		    int32_endian_swap(m_contentsSectionIndex);
		    int32_endian_swap(m_contentsSectionOffset);

		    int32_endian_swap(m_contentsClassNameSectionIndex);
		    int32_endian_swap(m_contentsClassNameSectionOffset);

		    int32_endian_swap(m_flags);

		    char_endian_swap(8, 4);
		    char_endian_swap(12, 4);
		    char_endian_swap(20, 4);
		    char_endian_swap(24, 4);
		    char_endian_swap(28, 4);
		    char_endian_swap(32, 4);
		    char_endian_swap(36, 4);

		    out_file_data[17]=1;
		}

		void print() {
		    lprintf("File number sections: %d\n", m_numSections);
		    lprintf("Content Section Index: %d\n", m_contentsSectionIndex);
		    lprintf("Content Section Offset: %d\n", m_contentsSectionOffset);

		    lprintf("Content Class Section Index: %d\n", m_contentsClassNameSectionIndex);
		    lprintf("Content Class Section Offset: %d\n", m_contentsClassNameSectionOffset);
		}
};



void convertElement(FILE *fp, string main_type) {
    if ((main_type == "int16") || (main_type == "uint16") || (main_type == "half")) {
        char_endian_swap(ftell(fp), 2);
        fseek(fp, 2, SEEK_CUR);
    }
    else if ((main_type == "int32") || (main_type == "uint32") || (main_type == "real") || (main_type == "pointer")  || (main_type == "ulong") || (main_type == "string_pointer") || (main_type == "cstring")) {
        char_endian_swap(ftell(fp), 4);
        fseek(fp, 4, SEEK_CUR);
    }
    else if (main_type == "variant") {
        char_endian_swap(ftell(fp), 4);
        char_endian_swap(ftell(fp)+4, 4);
        fseek(fp, 8, SEEK_CUR);
    }
    else if ((main_type == "int64") || (main_type == "uint64")) {
        char_endian_swap(ftell(fp), 8);
        fseek(fp, 8, SEEK_CUR);
    }
    else if ((main_type == "vector4") || (main_type == "quaternion")) {
        char_endian_swap(ftell(fp), 4);
        char_endian_swap(ftell(fp)+4, 4);
        char_endian_swap(ftell(fp)+8, 4);
        char_endian_swap(ftell(fp)+12, 4);
        fseek(fp, 16, SEEK_CUR);
    }
    else if ((main_type == "matrix3") || (main_type == "rotation") || (main_type == "qstransform")) {
        for (int j=0; j<12; j++) char_endian_swap(ftell(fp)+j*4, 4);

        fseek(fp, 4*12, SEEK_CUR);
    }
    else if ((main_type == "matrix4") || (main_type == "transform")) {
        for (int j=0; j<16; j++) char_endian_swap(ftell(fp)+j*4, 4);

        fseek(fp, 4*16, SEEK_CUR);
    }
    else if ((main_type=="array")) {
        char_endian_swap(ftell(fp), 4);
        char_endian_swap(ftell(fp)+4, 4);
        char_endian_swap(ftell(fp)+8, 4);
    }
    else fseek(fp, 1, SEEK_CUR);
}

void convertStructure(FILE *fp, string type_name) {
    unsigned int start=ftell(fp);
    bool found=false;

    for (list<hkType>::iterator it=hkTypes.begin(); it!=hkTypes.end(); it++) {
        if ((*it).name != type_name) continue;
        unsigned int array_offset=start+(*it).object_size;
        found = true;

        if ((*it).parent) convertStructure(fp, (*it).parent->name);

        for (int i=0; i<(*it).members.size(); i++) {
            string main_type=getTypeString((*it).members[i].tag[0]);
            string sub_type=getTypeString((*it).members[i].tag[1]);

            fseek(fp, start+(*it).members[i].offset, SEEK_SET);
            lprintf("Swapping endiannes of element at absolute offset %d(relative offset %d), Main type: %s Sub type: %s Reference: %s\n", start+(*it).members[i].offset, (int)(*it).members[i].offset, main_type.c_str(), sub_type.c_str(), (*it).members[i].structure.c_str());

            if (main_type == "enum") main_type = sub_type;

            if (main_type=="struct") convertStructure(fp, (*it).members[i].structure);
            else if (main_type=="pointer") {
                char_endian_swap(ftell(fp), 4);

                for (list<hkPointer>::iterator it2=data_pointers.begin(); it2!=data_pointers.end(); it2++) {
                    if (ftell(fp)==(*it2).abs_address) {
                        lprintf("Moving file pointer from %d to %d\n", (*it2).abs_address, (*it2).target_address);

                        fseek(fp, (*it2).target_address, SEEK_SET);
                        break;
                    }
                }

                /*
                if (sub_type == "struct") {
                    lprintf("Converting sub-structure %s\n", (*it).members[i].structure.c_str());
                    convertStructure(fp, (*it).members[i].structure);
                }
                else {
                    lprintf("Converting element %s\n", sub_type.c_str());
                    convertElement(fp, sub_type);
                }*/
            }
            else if ((main_type=="array") || (main_type=="simple_array")) {
                unsigned int count=0;

                fseek(fp, 4, SEEK_CUR);
                fread(&count, sizeof(unsigned int), 1, fp);
                uint32_endian_swap(count);

                char_endian_swap(ftell(fp)-8, 4);
                char_endian_swap(ftell(fp)-4, 4);
                if (main_type=="array") char_endian_swap(ftell(fp), 4);

                if (count == 0) {
                    lprintf("Array is void of any elements. Skipping.\n");
                    continue;
                }

                for (list<hkPointer>::iterator it2=data_pointers.begin(); it2!=data_pointers.end(); it2++) {
                    if ((ftell(fp)-8)==(*it2).abs_address) {
                        lprintf("Moving file pointer for new array of size %d from %d to %d\n", count, (*it2).abs_address, (*it2).target_address);

                        fseek(fp, (*it2).target_address, SEEK_SET);
                        break;
                    }
                }

                unsigned int newaddr=ftell(fp);
                unsigned int sz=1;

                for (list<hkType>::iterator it2=hkTypes.begin(); it2!=hkTypes.end(); it2++) {
                    if ((*it2).name == (*it).members[i].structure) {
                        sz = (*it2).object_size;
                        lprintf("Matching sub-structure size for array: %d\n", sz);
                        break;
                    }
                }

                for (int j=0; j<count; j++) {
                    if (sub_type == "struct") {
                        fseek(fp, newaddr+j*sz, SEEK_SET);
                        lprintf("Converting sub-structure %s %d\n", (*it).members[i].structure.c_str(), j);
                        convertStructure(fp, (*it).members[i].structure);
                    }
                    else {
                        if (sub_type == "pointer") {
                            /*for (list<hkPointer>::iterator it2=data_global_pointers.begin(); it2!=data_global_pointers.end(); it2++) {
                                if (ftell(fp)==(*it2).abs_address) {
                                    lprintf("Moving file pointer from %d to %d\n", (*it2).abs_address, (*it2).target_address);

                                    fseek(fp, (*it2).target_address, SEEK_SET);
                                    break;
                                }
                            }

                            convertStructure(fp, (*it).members[i].structure);*/
                        }
                        else {
                            lprintf("Converting element %s %d\n", sub_type.c_str(), j);
                            convertElement(fp, sub_type);
                        }
                    }
                }
            }
            else {
                unsigned int count=(*it).members[i].array_size;
                if (count == 0) count = 1;

                for (int j=0; j<count; j++) {
                    lprintf("Converting element %s %d sub-%d\n", main_type.c_str(), ftell(fp), j);
                    convertElement(fp, main_type);
                }
            }



        }
    }


    if (!found) {
        lprintf("**WARNING** Matching structure for %s couldn't be found in the types library. Is this HKX valid?\n", type_name.c_str());
        char c;
        cin >> c;
        lprintf("\n");
    }
}





/// Packfiles are composed of several sections.
/// A section contains several areas
/// | data | local | global | finish | exports | imports |
/// data: the user usable data.
/// local: pointer patches within this section (src,dst).
/// global: pointer patches to locations within this packfile (src,(section,dst)).
/// finish: offset and typename of all objects for finish functions (src, typename).
/// exports: named objects (src,name).
/// imports: named pointer patches outside this packfile (src,name).
class hkPackfileSectionHeader_conv {
	public:

			/// Default constructor
		hkPackfileSectionHeader_conv()
		{

		}

			/// Size in bytes of data part
		int getDataSize() const
		{
			return m_localFixupsOffset;
		}
			/// Size in bytes of intra section pointer patches
		int getLocalSize() const
		{
			return m_globalFixupsOffset - m_localFixupsOffset;
		}
			/// Size in bytes of inter section pointer patches
		int getGlobalSize() const
		{
			return m_virtualFixupsOffset - m_globalFixupsOffset;
		}
			/// Size in bytes of finishing table.
		int getFinishSize() const
		{
			return m_exportsOffset - m_virtualFixupsOffset;
		}
			/// Size in bytes of exports table.
		int getExportsSize() const
		{
			return m_importsOffset - m_exportsOffset;
		}
			/// Size in bytes of imports table.
		int getImportsSize() const
		{
			return m_endOffset - m_importsOffset;
		}

	public:

        ///
		char m_sectionTag[19];

        ///
		char m_nullByte;

        /// Absolute file offset of where this sections data begins.
		unsigned int m_absoluteDataStart;

        /// Offset of local fixups from absoluteDataStart.
		unsigned int m_localFixupsOffset;

        /// Offset of global fixups from absoluteDataStart.
		unsigned int m_globalFixupsOffset;

        /// Offset of virtual fixups from absoluteDataStart.
		unsigned int m_virtualFixupsOffset;

        /// Offset of exports from absoluteDataStart.
		unsigned int m_exportsOffset;

        /// Offset of imports from absoluteDataStart.
		unsigned int m_importsOffset;

        /// Offset of the end of section. Also the section size.
		unsigned int m_endOffset;

		void convertToPC() {
		    uint32_endian_swap(m_absoluteDataStart);
		    uint32_endian_swap(m_localFixupsOffset);
		    uint32_endian_swap(m_globalFixupsOffset);
		    uint32_endian_swap(m_virtualFixupsOffset);
		    uint32_endian_swap(m_exportsOffset);
		    uint32_endian_swap(m_importsOffset);
		    uint32_endian_swap(m_endOffset);
		}

		void print() {
		    lprintf("\n\nSection name: %s\n", m_sectionTag);
		    lprintf("Absolute file offset: %u\n", m_absoluteDataStart);
		    lprintf("Offset of local fixups: %u\n", m_localFixupsOffset);
		    lprintf("Offset of global fixups: %u\n", m_globalFixupsOffset);
		    lprintf("Offset of virtual fixups: %u\n", m_virtualFixupsOffset);
		    lprintf("Offset of exports: %u\n", m_exportsOffset);
		    lprintf("Offset of imports: %u\n", m_importsOffset);
		    lprintf("Offset of the end of section/section size: %u\n\n\n", m_endOffset);
		}

		void readData(FILE *fp) {
            fseek(fp, m_absoluteDataStart, SEEK_SET);

            // CLASSNAMES
            if (!strcmp(m_sectionTag, "__classnames__")) {
                hkClassName classname;

                class_name_global_address=m_absoluteDataStart;

                int i=0;
                while (classname.tag != (unsigned int)-1) {
                    classname.name="";

                    fread(&classname.tag, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(classname.tag);
                    char_endian_swap(ftell(fp)-4, 4);


                    if ((classname.tag == (unsigned int)-1) || (classname.tag == 0)) {
                        break;
                    }
                    else {
                        fseek(fp, 1, SEEK_CUR);
                        char c=0;
                        for (int i=0; i<256; i++) {
                            fread(&c, sizeof(char), 1, fp);
                            if (!c) break;

                            classname.name += c;
                        }

                        lprintf("Class Name #%d: %u %s\n", i, classname.tag, classname.name.c_str());
                        hkClassNames.push_back(classname);
                    }

                    i++;
                }
            }


            // TYPES
            if (!strcmp(m_sectionTag, "__types__")) {
                int i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_localFixupsOffset+i*4, SEEK_SET);

                    unsigned int address=0;
                    fread(&address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(address);
                    if (address==(unsigned int)-1) break;

                    char_endian_swap(ftell(fp)-4, 4);

                    //lprintf("New local fixup type entry #%d: Address: %d\n", i, m_absoluteDataStart+address);

                    if (ftell(fp) >= m_absoluteDataStart+m_globalFixupsOffset) break;

                    i++;
                }

                i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_globalFixupsOffset+i*12, SEEK_SET);

                    unsigned int address=0;
                    unsigned int type=0;
                    unsigned int meta_address=0;
                    fread(&address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(address);
                    if (address==(unsigned int)-1) break;

                    fread(&type, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(type);

                    fread(&meta_address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(meta_address);

                    hkLink link;
                    link.address_1=m_absoluteDataStart+address;
                    link.address_2=m_absoluteDataStart+meta_address;
                    link.type=type;
                    type_links.push_back(link);

                    char_endian_swap(ftell(fp)-12, 4);
                    char_endian_swap(ftell(fp)-8, 4);
                    char_endian_swap(ftell(fp)-4, 4);

                    if (ftell(fp) >= m_absoluteDataStart+m_virtualFixupsOffset) break;

                    i++;
                }

                i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_virtualFixupsOffset+i*12, SEEK_SET);

                    unsigned int address=0;
                    unsigned int name_address=0;
                    string type_name="";

                    fread(&address, sizeof(unsigned int), 1, fp);
                    fseek(fp, 4, SEEK_CUR);

                    if (((address==0) && (i != 0)) || (address == (unsigned int)-1)) break;

                    fread(&name_address, sizeof(unsigned int), 1, fp);

                    uint32_endian_swap(address);
                    uint32_endian_swap(name_address);

                    char_endian_swap(ftell(fp)-4, 4);
                    char_endian_swap(ftell(fp)-8, 4);
                    char_endian_swap(ftell(fp)-12, 4);


                    fseek(fp, class_name_global_address+name_address, SEEK_SET);
                    for (int k=0; k<256; k++) {
                        char c=0;
                        fread(&c, sizeof(char), 1, fp);
                        if (c) type_name += c;
                        else break;
                    }

                    fseek(fp, m_absoluteDataStart+address, SEEK_SET);
                    //lprintf("\n\nNew type entry #%d: File absolute address - %d  Class Name - %s\n", i, m_absoluteDataStart+address, type_name.c_str());


                    hkType type;
                    type.reset();
                    type.address = m_absoluteDataStart+address;
                    type.class_name = type_name;

                    if ((type_name != "hkClass") && (type_name != "hkClassEnum")) {
                        i++;
                        continue;
                    }

                    fseek(fp, 4, SEEK_CUR);

                    for (list<hkLink>::iterator it=type_links.begin(); it!=type_links.end(); it++) {
                        if ((*it).address_1 == ftell(fp)) {
                            type.parent_address = (*it).address_2;
                            break;
                        }
                    }


                    fseek(fp, 4, SEEK_CUR);
                    fread(&type.object_size, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(type.object_size);
                    char_endian_swap(ftell(fp)-4, 4);


                    fread(&type.num_implemented_interfaces, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(type.num_implemented_interfaces);
                    char_endian_swap(ftell(fp)-4, 4);

                    fseek(fp, 4, SEEK_CUR);

                    fread(&type.declared_enums, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(type.declared_enums);
                    char_endian_swap(ftell(fp)-4, 4);

                    fseek(fp, 4, SEEK_CUR);

                    unsigned int membernum=0;
                    fread(&membernum, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(membernum);
                    char_endian_swap(ftell(fp)-4, 4);


                    if (type_name == "hkClass") {
                        fseek(fp, 12, SEEK_CUR);
                        fread(&type.described_version, sizeof(unsigned int), 1, fp);
                        uint32_endian_swap(type.described_version);
                        char_endian_swap(ftell(fp)-4, 4);
                    }

                    char c=0;
                    type.name="";
                    for (int k=0; k<256; k++) {
                        fread(&c, sizeof(char), 1, fp);
                        if (!c) break;

                        type.name += c;
                    }


                    fixFileAlign(fp);

                    if (type_name == "hkClass") {
                        // Sub enumerators
                        vector<int> subs_sizes;
                        for (int j=0; j<type.declared_enums; j++) {
                            fseek(fp, 8, SEEK_CUR);
                            int sz=0;
                            fread(&sz, sizeof(int), 1, fp);
                            int32_endian_swap(sz);
                            char_endian_swap(ftell(fp)-4, 4);

                            fseek(fp, 8, SEEK_CUR);

                            subs_sizes.push_back(sz);
                        }

                        for (int j=0; j<type.declared_enums; j++) {
                            string nm="";
                            for (int k=0; k<256; k++) {
                                char c=0;
                                fread(&c, sizeof(char), 1, fp);

                                if (c) nm += c;
                                else break;
                            }
                            fixFileAlign(fp);


                            vector<hkEnum> subs;
                            subs.clear();
                            type.sub_enum_names.push_back(nm);

                            fixFileAlign(fp);

                            lprintf("\n\nSub enumeration list: %s Elements: %d\n", nm.c_str(), subs_sizes[j]);

                            hkEnum en;
                            for (int x=0; x<subs_sizes[j]; x++) {
                                fread(&en.id, sizeof(unsigned int), 1, fp);
                                uint32_endian_swap(en.id);
                                char_endian_swap(ftell(fp)-4, 4);

                                en.name="";
                                fseek(fp, 4, SEEK_CUR);

                                subs.push_back(en);
                            }

                            fixFileAlign(fp);

                            for (int x=0; x<subs_sizes[j]; x++) {
                                for (int k=0; k<256; k++) {
                                    char c=0;
                                    fread(&c, sizeof(char), 1, fp);

                                    if (c) subs[x].name += c;
                                    else break;
                                }

                                fixFileAlign(fp);

                                lprintf("Enum for sub list #%d: %d %s\n", j, subs[x].id, subs[x].name.c_str());

                                type.sub_enums.push_back(subs);
                            }
                        }


                        // Type members
                        hkTypeMember typemember;
                        for (int j=0; j<membernum; j++) {
                            fseek(fp, 4, SEEK_CUR);

                            typemember.structure_address = 0;

                            for (list<hkLink>::iterator it=type_links.begin(); it!=type_links.end(); it++) {
                                if ((*it).address_1 == ftell(fp)) {
                                    typemember.structure_address = (*it).address_2;
                                    break;
                                }
                            }

                            fseek(fp, 8, SEEK_CUR);

                            fread(typemember.tag, sizeof(char), 2, fp);

                            fread(&typemember.array_size, sizeof(unsigned short), 1, fp);
                            uint16_endian_swap(typemember.array_size);

                            fread(&typemember.struct_type, sizeof(unsigned short), 1, fp);
                            uint16_endian_swap(typemember.struct_type);

                            fread(&typemember.offset, sizeof(unsigned short), 1, fp);
                            uint16_endian_swap(typemember.offset);

                            char_endian_swap(ftell(fp)-6, 2);
                            char_endian_swap(ftell(fp)-4, 2);
                            char_endian_swap(ftell(fp)-2, 2);

                            typemember.name="";
                            typemember.structure = "";

                            type.members.push_back(typemember);
                            fseek(fp, 4, SEEK_CUR);
                        }



                        for (int j=0; j<membernum; j++) {
                            type.members[j].name = "";
                            for (int k=0; k<256; k++) {
                                char c=0;
                                fread(&c, sizeof(char), 1, fp);

                                if (c) type.members[j].name += c;
                                else break;
                            }

                            fixFileAlign(fp);
                        }
                    }
                    else {
                        hkEnum en;
                        lprintf("\n\nNew enumeration list %s\n", type.name.c_str());

                        for (int j=0; j<type.object_size; j++) {
                            fread(&en.id, sizeof(unsigned int), 1, fp);
                            uint32_endian_swap(en.id);
                            char_endian_swap(ftell(fp)-4, 4);

                            en.name="";

                            fseek(fp, 4, SEEK_CUR);

                            type.enums.push_back(en);
                        }
                        fixFileAlign(fp);


                        for (int j=0; j<type.object_size; j++) {
                            for (int k=0; k<256; k++) {
                                char c=0;
                                fread(&c, sizeof(char), 1, fp);

                                if (c) type.enums[j].name += c;
                                else break;
                            }

                            fixFileAlign(fp);


                            lprintf("New enum: %d %s\n", type.enums[j].id, type.enums[j].name.c_str());
                        }
                    }

                    hkTypes.push_back(type);
                    i++;
                }

                i =0;
                // Link types
                for (list<hkLink>::iterator it=type_links.begin(); it!=type_links.end(); it++) {
                    (*it).type_parent=NULL;
                    (*it).type_node=NULL;

                    if ((*it).type == 0) lprintf("New type0 link #%d: %d %d\n", i, (*it).address_1, (*it).address_2);
                    if ((*it).type == 1) lprintf("New type1 link #%d: %d %d\n", i, (*it).address_1, (*it).address_2);

                    i++;
                }

                int j=0;
                for (list<hkType>::iterator it=hkTypes.begin(); it!=hkTypes.end(); it++) {
                    if ((*it).class_name != "hkClass") continue;

                    if ((*it).parent_address) {
                        for (list<hkType>::iterator it2=hkTypes.begin(); it2!=hkTypes.end(); it2++) {
                            if ((*it2).address == (*it).parent_address) {
                                (*it).parent = &(*it2);
                                lprintf("Assigning parent of type %s\n", (*it2).name.c_str());
                                break;
                            }
                        }
                    }

                    for (int x=0; x<(*it).members.size(); x++) {
                        if ((*it).members[x].structure_address) {
                            for (list<hkType>::iterator it2=hkTypes.begin(); it2!=hkTypes.end(); it2++) {
                                if ((*it2).address == (*it).members[x].structure_address) {
                                    (*it).members[x].structure = (*it2).name;
                                    lprintf("Assigning structure of type %s to type member %s(relative offset: %d)\n", (*it2).name.c_str(), (*it).members[x].name.c_str(), (*it).members[x].offset);
                                    break;
                                }
                            }
                        }
                    }

                    lprintf("New Type #%d: %s  Size: %d  Declared Enums: %d  Implemented interfaces: %d  Declared Members: %d\n", j,(*it).name.c_str(), (*it).object_size, (*it).declared_enums, (*it).num_implemented_interfaces, (*it).members.size());
                    if ((*it).parent) lprintf("* Parent type is %s(Size: %d)\n", (*it).parent->name.c_str(), (*it).parent->object_size);

                    for (int x=0; x<(*it).members.size(); x++) {
                        lprintf("Type member: %s - Type: %s %s Sub-type: %s  Offset: %d  Array Size: %d Structure Type: %d\n", (*it).members[x].name.c_str(), getTypeString((int)(*it).members[x].tag[0]).c_str(), ((*it).members[x].structure != "" ? ("of "+(*it).members[x].structure).c_str() : ""), getTypeString((int)(*it).members[x].tag[1]).c_str(), (int)(*it).members[x].offset, (unsigned int)(*it).members[x].array_size, (unsigned int)(*it).members[x].struct_type);
                    }
                    lprintf("\n\n");

                    j++;
                }
            }

            // Data
            if (!strcmp(m_sectionTag, "__data__")) {
                int i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_localFixupsOffset+i*8, SEEK_SET);

                    unsigned int address=0;
                    unsigned int address_2=0;
                    fread(&address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(address);
                    if (address==(unsigned int)-1) break;

                    fread(&address_2, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(address_2);

                    char_endian_swap(ftell(fp)-8, 4);
                    char_endian_swap(ftell(fp)-4, 4);



                    hkPointer pointer;

                    pointer.abs_address=address+m_absoluteDataStart;
                    pointer.target_address=address_2+m_absoluteDataStart;

                    data_pointers.push_back(pointer);

                    lprintf("New local fixup data entry #%d: Address of pointer: %d  Pointer data: %d\n", i, pointer.abs_address, pointer.target_address);

                    if (ftell(fp) >= m_absoluteDataStart+m_globalFixupsOffset) break;

                    i++;
                }

                i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_globalFixupsOffset+i*12, SEEK_SET);

                    unsigned int address=0;
                    unsigned int type=0;
                    unsigned int meta_address=0;
                    fread(&address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(address);
                    if (address==(unsigned int)-1) break;

                    fread(&type, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(type);

                    fread(&meta_address, sizeof(unsigned int), 1, fp);
                    uint32_endian_swap(meta_address);


                    char_endian_swap(ftell(fp)-12, 4);
                    char_endian_swap(ftell(fp)-8, 4);
                    char_endian_swap(ftell(fp)-4, 4);

                    hkPointer pointer;
                    pointer.abs_address=address+m_absoluteDataStart;
                    pointer.target_address=meta_address+m_absoluteDataStart;
                    data_global_pointers.push_back(pointer);

                    if (ftell(fp) >= m_absoluteDataStart+m_virtualFixupsOffset) break;

                    lprintf("New global fixup data entry #%d: Address: %d  Type: %d  Meta Address: %d\n", i, m_absoluteDataStart+address, type, m_absoluteDataStart+meta_address);
                    i++;
                }


                i=0;
                while (true) {
                    fseek(fp, m_absoluteDataStart+m_virtualFixupsOffset+i*12, SEEK_SET);

                    unsigned int address=0;
                    unsigned int name_address=0;
                    string type_name="";

                    fread(&address, sizeof(unsigned int), 1, fp);
                    fseek(fp, 4, SEEK_CUR);
                    uint32_endian_swap(address);
                    if (address==(unsigned int)-1) break;

                    fread(&name_address, sizeof(unsigned int), 1, fp);

                    char_endian_swap(ftell(fp)-4, 4);
                    char_endian_swap(ftell(fp)-8, 4);
                    char_endian_swap(ftell(fp)-12, 4);


                    uint32_endian_swap(name_address);
                    unsigned back=ftell(fp);


                    fseek(fp, class_name_global_address+name_address, SEEK_SET);
                    for (int k=0; k<256; k++) {
                        char c=0;
                        fread(&c, sizeof(char), 1, fp);
                        if (c) type_name += c;
                        else break;
                    }

                    lprintf("New data entry #%d at %d: Address: %d  Class: %s\n", i, back, m_absoluteDataStart+address, type_name.c_str());

                    fseek(fp, m_absoluteDataStart+address, SEEK_SET);

                    convertStructure(fp, type_name);

                    if (back >= m_absoluteDataStart+m_exportsOffset) break;

                    i++;
                }
            }

		}
};




void loadTypesDatabase(string filename) {
    FILE *fp=fopen(filename.c_str(), "rb");
	if (!fp) {
	    lprintf("No existing %s to load as Types database.\n", filename.c_str());
	    return;
	}
	fclose(fp);


	TiXmlDocument doc(filename);
	doc.LoadFile();

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlHandle hRoot(0);


	pElem=hDoc.FirstChildElement().Element();

	// <Shader>
	for( pElem; pElem; pElem=pElem->NextSiblingElement())
	{
		const char *pKey=pElem->Value();
		const char *pText=pElem->GetText();
/*
		if (!strcmp(pKey, "Type")) {
		    hkType type;
			pElem->QueryStringAttribute("name", &type.name);

			TiXmlElement* pElem_i;
			pElem_i=pElem->FirstChildElement();

			// <Definition>
			for( pElem_i; pElem_i; pElem_i=pElem_i->NextSiblingElement())
			{
				const char *pKey_i=pElem_i->Value();
				const char *pText_i=pElem_i->GetText();

				if (!strcmp(pKey_i, "Definition")) {
					float r=0.0;
					float g=0.0;
					float b=0.0;
					float a=0.0;

					mat->material_definition_names.push_back(ToString(pText_i));

					pElem_i->QueryFloatAttribute("r", &r);
					pElem_i->QueryFloatAttribute("g", &g);
					pElem_i->QueryFloatAttribute("b", &b);
					pElem_i->QueryFloatAttribute("a", &a);

					mat->material_definition_rgbs.push_back(Ogre::Vector3(r, g, b));
					mat->material_definition_as.push_back(a);
				}
			}

			terrain_material_templates.push_back(mat);
		}

		if (!strcmp(pKey, "TextureUnit")) {
			terrain_material_texture_units.push_back(ToString(pText));
		}
*/
	}
}

void saveTypesDatabase(string filename) {
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild( decl );


	for (list<hkType>::iterator it=hkTypes.begin(); it!=hkTypes.end(); it++) {
	    if ((*it).class_name != "hkClass") continue;


/*
        vector<hkTypeMember> members;
        vector<hkEnum> enums;
        vector< vector<hkEnum> > sub_enums;
        vector<string> sub_enum_names;
*/

		TiXmlElement *newElem = new TiXmlElement("Type");

		newElem->SetAttribute("name", (*it).name);
		if ((*it).parent) newElem->SetAttribute("parent_name", (*it).parent->name);
		else newElem->SetAttribute("parent_name", "NULL");

		newElem->SetAttribute("object_size", (*it).object_size);
		newElem->SetAttribute("described_version", (*it).described_version);
		newElem->SetAttribute("num_implemented_interfaces", (*it).num_implemented_interfaces);
		newElem->SetAttribute("declared_enums", (*it).declared_enums);


		for (int i=0; i<(*it).members.size(); i++) {
			TiXmlElement *definition = new TiXmlElement("Member");

			TiXmlText *text = new TiXmlText((*it).members[i].name);
			definition->LinkEndChild(text);

			definition->SetAttribute("Type", (int)(*it).members[i].tag[0]);
			definition->SetAttribute("Sub-Type", (int)(*it).members[i].tag[1]);
			definition->SetAttribute("Array Size", (int)(*it).members[i].array_size);
			definition->SetAttribute("Structure", (int)(*it).members[i].struct_type);

			definition->SetAttribute("offset", (*it).members[i].offset);
			definition->SetAttribute("structure", (*it).members[i].structure);

			newElem->LinkEndChild(definition);
		}

		doc.LinkEndChild(newElem);
	}

	doc.SaveFile(filename.c_str());
}

int main(int argc, char** argv) {
    char *data=NULL;

    if (argc < 2) {
        printf("Usage: hkxconverter source.hkx target.hkx\n\n- If the output name isn't available, it writes out to source.hkx.out.");
        return 1;
    }

    FILE *src=fopen(argv[1], "rb");
    if (!src) {
        lprintf("Couldn't open %s, file doesn't exist.\n", "source.hkx");
        return 1;
    }

    FILE *dest=NULL;
    if (argc > 2) dest=fopen(argv[2], "wb");
    else dest=fopen((ToString(argv[1])+".out").c_str(), "wb");

    if (!dest) {
        lprintf("Couldn't create %s for writing, is the directory write-protected?\n", "out.hkx");
        return 1;
    }

    public_log=fopen("log.txt", "wt");
    if (!public_log) {
        lprintf("Couldn't create %s for writing, is the directory write-protected?\n", "log.txt");
        return 1;
    }

    //loadTypesDatabase("TypesDB.xml");

    fseek(src, 0L, SEEK_END);
    int sz=ftell(src);
    fseek(src, 0L, SEEK_SET);
    out_file_data = new char[sz];
    fread(out_file_data, sizeof(char), sz, src);
    fseek(src, 0L, SEEK_SET);


    hkPackfileHeader_conv header;

    fread(&header, sizeof(hkPackfileHeader_conv), 1, src);
    header.convertToPC();
    header.print();

    for (int i=0; i<header.m_numSections; i++) {
        hkPackfileSectionHeader_conv section_header;
        fread(&section_header, sizeof(hkPackfileSectionHeader_conv), 1, src);
        section_header.convertToPC();

        char_endian_swap(ftell(src)-28, 4);
        char_endian_swap(ftell(src)-24, 4);
        char_endian_swap(ftell(src)-20, 4);
        char_endian_swap(ftell(src)-16, 4);
        char_endian_swap(ftell(src)-12, 4);
        char_endian_swap(ftell(src)-8, 4);
        char_endian_swap(ftell(src)-4, 4);

        section_header.print();

        unsigned int address=ftell(src);

        section_header.readData(src);

        fseek(src, address, SEEK_SET);
    }


    fwrite(out_file_data, sizeof(char), sz, dest);
    fclose(src);
    fclose(dest);

    //saveTypesDatabase("TypesDB.xml");
    return 0;
}
