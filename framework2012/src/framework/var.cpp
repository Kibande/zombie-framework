
#include "framework.hpp"

#include <littl/HashMap.hpp>

namespace zfw
{
    //static HashMap<String, Var_t> globalpool;

    static Var_t *first_var = nullptr, *last_var = nullptr;

    static Var_t* addVar( Var_t* var )
    {
        if ( first_var != nullptr )
        {
            last_var->next = var;
            var->prev = last_var;

            last_var = var;
        }
        else
        {
            first_var = var;
            last_var = var;
        }

        return var;
    }

    Var_t::Var_t( int flags, const char* name, const char* text )
            : flags( VAR_UTF8 | flags ), name( name ), text( text ), prev( nullptr ), next( nullptr )
    {
    }

    Var_t::Var_t( int flags, const char* name, int value )
            : flags( VAR_INT | flags ), name( name ), prev( nullptr ), next( nullptr )
    {
        basictypes.i = value;
    }

    bool Var_t::ConvertToFloat()
    {
        if ( (flags & VAR_FLOAT) )
            return true;
        
        if ( flags & VAR_LOCKTYPE )
            return false;
        
        if ( (flags & VAR_INT) )
            basictypes.f = (float) basictypes.i;
        else if ( ( flags & VAR_UTF8 ) )
            basictypes.f = text.toFloat();
        
        return true;
    }

    bool Var_t::ConvertToInt()
    {
        if ( (flags & VAR_INT) )
            return true;

        if ( flags & VAR_LOCKTYPE )
            return false;

        if ( (flags & VAR_FLOAT) )
            basictypes.i = ( int ) basictypes.f;
        else if ( ( flags & VAR_UTF8 ) )
            basictypes.i = text.toInt();

        return true;
    }

    bool Var_t::ConvertToStr()
    {
        if ( (flags & VAR_UTF8) )
            return true;

        if ( flags & VAR_LOCKTYPE )
            return false;

        if ( (flags & VAR_INT) )
            text = ( String&& ) String::formatInt( basictypes.i );
        else if ( (flags & VAR_FLOAT) )
            text = ( String&& ) String::formatFloat( basictypes.f );

        return true;
    }

    int Var_t::GetInt()
    {
        if ( (flags & VAR_INT) )
            return basictypes.i;
        else if ( (flags & VAR_FLOAT) )
            return ( int ) basictypes.f;
        else if ( ( flags & VAR_UTF8 ) )
            return text.toInt();
        else
            return -1;
    }

    const char* Var_t::GetStr()
    {
        if ( (flags & VAR_INT) )
            text = ( String&& ) String::formatInt( basictypes.i );
        else if ( (flags & VAR_FLOAT) )
            text = ( String&& ) String::formatFloat( basictypes.f );

        return text;
    }

    void Var_t::LockType()
    {
        flags &= VAR_LOCKTYPE;
    }

    void Var_t::Print()
    {
        printf( "%s\t", name.c_str() );

        if ( (flags & VAR_UTF8) )
            printf( "%s\n", text.c_str() );
        else if ( (flags & VAR_INT) )
            printf( "%i\n", basictypes.i );
        else if ( (flags & VAR_FLOAT) )
            printf( "%g\n", basictypes.f );
        else
            printf( "??\n" );
    }

    void Var_t::SetFloat( float value )
    {
        if ( (flags & VAR_FLOAT) || !(flags & VAR_LOCKTYPE) )
        {
            flags = (flags & ~VAR_TYPEMASK) | VAR_FLOAT;
            basictypes.f = value;
            
        }
        else if ( (flags & VAR_UTF8) )
            text = String::formatFloat( value );
        else if ( (flags & VAR_INT) )
            basictypes.i = ( int )( value );
    }
    
    void Var_t::SetInt( int value )
    {
        if ( (flags & VAR_INT) || !(flags & VAR_LOCKTYPE) )
        {
            flags = (flags & ~VAR_TYPEMASK) | VAR_INT;
            basictypes.i = value;
            
        }
        else if ( (flags & VAR_UTF8) )
            text = String::formatInt( value );
        else if ( (flags & VAR_FLOAT) )
            basictypes.f = ( float )( value );
    }

    void Var_t::SetStr( const char* value )
    {
        if ( (flags & VAR_UTF8) || !(flags & VAR_LOCKTYPE) )
        {
            flags = (flags & ~VAR_TYPEMASK) | VAR_UTF8;
            text = value;

        }
        else if ( (flags & VAR_INT) )
            basictypes.i = String::toInt( value );
        else if ( (flags & VAR_FLOAT) )
            basictypes.f = String::toFloat( value );
    }

    void Var::Clear()
    {
        while ( first_var != nullptr )
        {
            Var_t* var = first_var;
            first_var = first_var->next;

            delete var;
        }

        last_var = nullptr;
    }

    void Var::Dump()
    {
        /*iterate2 ( i, globalpool )
            printf( "%s\t%s\n", ( *i ).key.c_str(), (*i).value.GetStr() );

        globalpool.printStatistics();*/

        // TODO: printDump()
    }

    Var_t* Var::Find( const char* name )
    {
        //return globalpool.find( (String&&) name );

        Var_t* var = first_var;

        while ( var != nullptr )
        {
            if ( String::equals( name, var->name, false ) )
                return var;

            var = var->next;
        }

        return nullptr;
    }

    bool Var::GetInt( const char* name, int* i, bool required )
    {
        Var_t* var = Var::Find( name );

        if ( var == nullptr )
        {
            if ( required )
                Sys::RaiseException( EX_VARIABLE_UNDEFINED, "Var::GetStr", "undefined variable '%s'", name );

            return false;
        }

        *i = var->GetInt();
        return true;
    }

    int Var::GetInt( const char* name, bool required, int def  )
    {
        int value = def;

        Var::GetInt( name, &value, required );

        return value;
    }

    const char* Var::GetStr( const char* name, bool required )
    {
        Var_t* var = Var::Find( name );

        if ( var == nullptr )
        {
            if ( required )
                Sys::RaiseException( EX_VARIABLE_UNDEFINED, "Var::GetStr", "undefined variable '%s'", name );

            return nullptr;
        }

        return var->GetStr();
    }

    Var_t* Var::Lock( const char* name, bool lockvalue, int defaultval )
    {
        Var_t* var = Var::Find( name );
        
        if ( var == nullptr )
            var = Var::SetInt( name, defaultval );
        else if ( lockvalue && (var->flags & VAR_LOCKED) )
        {
            Sys::RaiseException( EX_VARIABLE_LOCKED, "Var::Lock", "variable '%s' is already locked", name );
            return nullptr;
        }
        
        if ( lockvalue )
            var->flags |= VAR_LOCKED;
        
        return var;
    }
    
    Var_t* Var::Lock( const char* name, bool lockvalue, float defaultval )
    {
        Var_t* var = Var::Find( name );

        if ( var == nullptr )
            var = Var::SetFloat( name, defaultval );
        else if ( lockvalue && (var->flags & VAR_LOCKED) )
        {
            Sys::RaiseException( EX_VARIABLE_LOCKED, "Var::Lock", "variable '%s' is already locked", name );
            return nullptr;
        }

        if ( lockvalue )
            var->flags |= VAR_LOCKED;

        return var;
    }

    Var_t* Var::LockFloat( const char* name, bool lockvalue, float defaultval )
    {
        Var_t* var = Var::Lock( name, lockvalue, defaultval );
        
        if ( !(var->flags & VAR_FLOAT) && !var->ConvertToFloat() )
        {
            Sys::RaiseException( EX_VARIABLE_TYPE_LOCKED, "Var::LockFloat", "variable '%s' is already type-locked as a different type", name );
            return nullptr;
        }
        
        var->flags |= VAR_LOCKTYPE;
        
        return var;
    }

    Var_t* Var::LockInt( const char* name, bool lockvalue, int defaultval )
    {
        Var_t* var = Var::Lock( name, lockvalue, defaultval );

        if ( !(var->flags & VAR_INT) && !var->ConvertToInt() )
        {
            Sys::RaiseException( EX_VARIABLE_TYPE_LOCKED, "Var::LockInt", "variable '%s' is already type-locked as a different type", name );
            return nullptr;
        }

        var->flags |= VAR_LOCKTYPE;

        return var;
    }

    /*Var_t* Var::LockStr( const char* name )
    {
        Var_t* var = Var::Lock( name );

        if ( !(var->flags & VAR_UTF8) )
        {
            if ( (var->flags & VAR_LOCKTYPE) )
            {
                Sys::FatalError( "Var::LockStr", "variable '%s' already type-locked as a different type", name );
                return nullptr;
            }

            var->ConvertToStr();
        }

        var->flags |= VAR_LOCKED;

        return var;
    }*/

    /*Var_t* Var::TypeLockString( const char* name )
    {
        Var_t* var = Var::Find( name );

        if ( var == nullptr )
            return Var::SetString( name, "" );
        else if ( !(var->flags & VAR_UTF8) )
        {
            if ( (var->flags & VAR_LOCKTYPE) )
            {
                Sys::FatalError( "Var::TypeLockString", "variable '%s' already type-locked as a different type", name );
                return nullptr;
            }

            var->ConvertToStr();
        }

        var->flags |= VAR_TYPELOCK;

        return var;
    }*/

    Var_t* Var::SetFloat( const char* name, float value )
    {
        Var_t* var = Var::Find( name );
        
        if ( var != nullptr )
        {
            if ( var->flags & VAR_LOCKED )
            {
                Sys::RaiseException( EX_VARIABLE_LOCKED, "Var::SetFloat", "variable '%s' is locked", name );
                return nullptr;
            }
            
            var->SetFloat( value );
            return var;
        }
        else
            //return globalpool.set( (String&&) name, Var_t( 0, name, value ) );
            return addVar( new Var_t( 0, name, value ) );
    }
    
    Var_t* Var::SetInt( const char* name, int value )
    {
        Var_t* var = Var::Find( name );

        if ( var != nullptr )
        {
            if ( var->flags & VAR_LOCKED )
            {
                Sys::RaiseException( EX_VARIABLE_LOCKED, "Var::SetInt", "variable '%s' is locked", name );
                return nullptr;
            }

            var->SetInt( value );
            return var;
        }
        else
            //return globalpool.set( (String&&) name, Var_t( 0, name, value ) );
            return addVar( new Var_t( 0, name, value ) );
    }

    Var_t* Var::SetStr( const char* name, const char* value )
    {
        Var_t* var = Var::Find( name );

        if ( var != nullptr )
        {
            if ( var->flags & VAR_LOCKED )
            {
                Sys::RaiseException( EX_VARIABLE_LOCKED, "Var::SetStr", "variable '%s' is locked", name );
                return nullptr;
            }

            var->SetStr( value );
            return var;
        }
        else
            //return globalpool.set( (String&&) name, Var_t( 0, name, value ) );
            return addVar( new Var_t( 0, name, value ) );
    }

    void Var::Unlock( Var_t*& var )
    {
        if ( var != nullptr )
        {
            var->flags &= ~(VAR_LOCKED | VAR_LOCKTYPE);
            var = nullptr;
        }
    }
}
