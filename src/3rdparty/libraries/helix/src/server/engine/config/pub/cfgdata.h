/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cfgdata.h,v 1.2 2003/01/23 23:42:53 damonlan Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

#ifndef _CFGDATA_H_
#define _CFGDATA_H_

enum CT_base_type
{
    CT_INT,
    CT_STRING,
    CT_LIST,
    CT_BOOLEAN,
    CT_STRUCT
};

const int CFGDATA_DEFAULT_SHORT_VALUE		= 0;
const int CFGDATA_SHORT_SIZE_IN_BYTES		= 2;
const int CFGDATA_LONG_SIZE_IN_BYTES		= 4;
const int CFGDATA_CHAR_SIZE_IN_BYTES		= 1;
const int CFGDATA_SIZE_SIZE_IN_BYTES		= 4;
const int CFGDATA_MAX_STRUCT_ELEMENT_NAME_LEN	= 256;
const int CFGDATA_MAX_NUM_LISTS			= 10;
const int CFGDATA_MAX_NUM_STRUCTS		= 10;

enum Config_id_type 
{ 
	CFGID_VAR,
	CFGID_TYPE
};

class Config_error;
class Config_var;
class Config_type;
class Config_int_type;
class Config_string_type;
class Config_list_type;
class Config_boolean_type;
class Config_struct_type;

class Config_id 
{
public:
    Config_id_type	    id();
    virtual		    ~Config_id();

protected:
			    Config_id(Config_id_type new_id);

private:
    Config_id_type	    _id;
};


class Config_var : public Config_id 
{
public:
    friend		    class Config_int_type;
    friend		    class Config_string_type;
    friend		    class Config_list_type;
    friend		    class Config_boolean_type;
    friend		    class Config_struct_type;

			    Config_var(Config_type* new_type);
    virtual		    ~Config_var();

    const char*		    name();
    void*		    value();
    Config_type*	    type();
    u_long32		    size();
    Config_error*	    set_name(const char* new_name);
    Config_error*	    set_value(void* new_value, u_long32 size);
    Config_var*		    get_element(int index);
    Config_var*		    get_element(const char* name);
    Config_error*	    set_element(int index, Config_var* value);
    Config_error*	    set_element(const char* name, Config_var* value);

    int			    _hidden;

protected:

    u_long32		    _size;
    void*		    _value;

private:
			    Config_var();
    void		    init_me();

    char*		    _name;
    Config_type*	    _type;
};

/*
 *
 * Type classes
 *
 
 */
class Config_type : public Config_id 
{
public:
    CT_base_type	    base_type();
    virtual Config_error*   set_value(Config_var* var, void* value, 
				      u_long32 size) = 0;
    virtual Config_var*     get_element(Config_var* var, int index);
    virtual Config_error*   set_element(Config_var* var, int index, 
					Config_var* element);
    virtual Config_error*   set_element(Config_var* var, const char* name,
					Config_var* element);
    virtual Config_var*     get_element(Config_var* var, const char* name);
    virtual void	    init(Config_var* var) = 0;
				
protected:
			    Config_type(CT_base_type base_type);
private:
    CT_base_type	    _base_type;
};

class Config_boolean_type: public Config_type 
{
public:
    Config_error*		set_value(Config_var* var, void* value, 
					  u_long32 size);
    static Config_boolean_type* instance();
    virtual void		init(Config_var *var);
private:
				Config_boolean_type();
    static Config_boolean_type* _instance;
};

class Config_int_type: public Config_type 
{
public:
    Config_error*	    set_value(Config_var* var, void* value,
				      u_long32 size);

    static Config_int_type* instance();
    virtual void	    init(Config_var* var);
private:
			    Config_int_type();
    static Config_int_type* _instance;
};

class Config_string_type: public Config_type 
{
public:
    Config_error*		set_value(Config_var* var, void* value, 
					  u_long32 size);

    static Config_string_type*	instance();
    virtual void		init(Config_var* var);
private:
				Config_string_type();
    static Config_string_type*	_instance;
};

class Config_list_type: public Config_type 
{
public:
    Config_error*		set_value(Config_var* var, void* value,
					  u_long32 size);

    static Config_list_type*	instance(Config_type* element_type);
    Config_var*			get_element(Config_var* var, int index);
    Config_error*		set_element(Config_var* var, int index, 
					    Config_var* new_value);
    Config_type*		element_type() { return _element_type; };
    virtual void		init(Config_var* var);

private:
				Config_list_type(Config_type* element_type);
    Config_type*		_element_type;
    static  Config_list_type**	_instance_list;
};

struct Config_struct_info 
{
    char		element_name[CFGDATA_MAX_STRUCT_ELEMENT_NAME_LEN];
    Config_type*	element_type;
};

class Config_struct_type: public Config_type 
{
public:
    Config_error*		set_value(Config_var* var, void* value, 
					  u_long32 size);

    static Config_struct_type*	instance(Config_struct_info* sturct_info, 
					 int num_of_elements);
    Config_var*			get_element(Config_var* var, const char* name);
    Config_error*		set_element(Config_var* var, const char* name,
					    Config_var* new_value);
    int				num_of_elements() { return _num_of_elements; };
    Config_type*		get_element_type(int index);
    const char*			element_name(int index);
    virtual void		init(Config_var* var);

private:
    Config_type*		get_element_type(const char* name);
    int				get_index(const char* name);
    int				compare_struct_info(Config_struct_info* info, 
						    int num_of_elements);
			    Config_struct_type(Config_struct_info* struct_info, 
					       int num_of_elements);
    Config_struct_info*		_struct_info;
    int				_num_of_elements;
    static Config_struct_type** _instance_list;
};

/*
 * Config_id
 *
 */

inline
Config_id::Config_id(Config_id_type new_id)
    : _id(new_id)
{
}

inline
Config_id_type
Config_id::id()
{
    return _id;
}

/*
 * Config_var
 *
 */

inline
const char* 
Config_var::name()
{
    return _name;
}

inline
void*
Config_var::value()
{
    return _value;
}

inline
Config_type* 
Config_var::type()
{
    return _type;
}

inline
u_long32
Config_var::size()
{
    return _size;
}

/*
 * Config_type
 *
 */

inline
CT_base_type
Config_type::base_type()
{
    return _base_type;
}

inline
Config_var*
Config_type::get_element(Config_var* var, int index)
{
    return 0;
}

inline
Config_error*
Config_type::set_element(Config_var* var, int index, Config_var* element)
{
    return new Config_error(CFG_INTERNAL_ERROR, "set element is not supported "
			    "on this kind of var\n");
}

inline
Config_error*
Config_type::set_element(Config_var* var, const char* name, Config_var* element)
{
    return new Config_error(CFG_INTERNAL_ERROR, "set element is not supported "
			    "on this kind of var\n");
}

inline
Config_var*
Config_type::get_element(Config_var* var, const char* name)
{
    return 0;
}

#endif /* _CFGDATA_H_ */

