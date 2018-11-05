/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

#pragma once

// HDKBuiltinTypes.h - c++ wrapper class for the HDK builtin types

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "hdk_xml.h"
#include "hdk_cli.h"

#if defined(HDK_CLI_CPP_USE_STL)
#  include <string>
#endif

namespace HDK
{
    ///
    /// \enum ClientError
    ///     Possible errors returned by the client methods.
    ///
    enum ClientError
    {
        ClientError_OK = HDK_CLI_Error_OK,                           /*!< No error */
        ClientError_RequestInvalid = HDK_CLI_Error_RequestInvalid,   /*!< Error validating the request. */
        ClientError_ResponseInvalid = HDK_CLI_Error_ResponseInvalid, /*!< Error validating the response. */
        ClientError_SoapFault = HDK_CLI_Error_SoapFault,             /*!< SOAP fault returned by server. */
        ClientError_XmlParse = HDK_CLI_Error_XmlParse,               /*!< Error parsing the XML SOAP response. */
        ClientError_HttpAuth = HDK_CLI_Error_HttpAuth,               /*!< Invalid HTTP Basic Authentication credentials. */
        ClientError_HttpUnknown = HDK_CLI_Error_HttpUnknown,         /*!< Unknown/Unexpected HTTP response code. */
        ClientError_Connection = HDK_CLI_Error_Connection,           /*!< Transport-layer connection error. */
        ClientError_OutOfMemory = HDK_CLI_Error_OutOfMemory,         /*!< Memory allocation failed. */
        ClientError_InvalidArg = HDK_CLI_Error_InvalidArg,           /*!< An argument is invalid. */
        ClientError_Unknown = HDK_CLI_Error_Unknown,                 /*!< Unknown error */

        ClientError_HnapMethod = 0x8000                              /*!< The SOAP method call succeeded and a non-success HNAP result was returned by the method */
    };

    ///
    /// \class IPv4 address type.
    ///      This object should be used when dealing with IPv4 addresses in the HDK.
    ///
    class IPv4Address
    {
    public:
        //
        // Constructors/Destructor.
        //
        IPv4Address() throw() :
            m_hdkipaddr(),
            m_fBlank(true)
        {
            memset(&m_hdkipaddr, 0, sizeof(m_hdkipaddr));
        }
        IPv4Address(const HDK_XML_IPAddress* phdkipaddr) throw() :
            m_hdkipaddr(),
            m_fBlank(0 == phdkipaddr)
        {
            if (0 != phdkipaddr)
            {
                m_hdkipaddr = *phdkipaddr;
            }
            else
            {
                memset(&m_hdkipaddr, 0, sizeof(m_hdkipaddr));
            }
        }
        IPv4Address(const IPv4Address & src) throw() :
            m_hdkipaddr(src.m_hdkipaddr),
            m_fBlank(src.m_fBlank)
        {
        }

        IPv4Address(const HDK_XML_IPAddress & hdkipaddr) throw() :
            m_hdkipaddr(hdkipaddr),
            m_fBlank(false)
        {
        }

        IPv4Address(unsigned long ulAddr) throw() :
            m_hdkipaddr(),
            m_fBlank(false)
        {
            m_hdkipaddr.a = (unsigned char)((ulAddr >> 24) & 0xff);
            m_hdkipaddr.b = (unsigned char)((ulAddr >> 16) & 0xff);
            m_hdkipaddr.c = (unsigned char)((ulAddr >> 8) & 0xff);
            m_hdkipaddr.d = (unsigned char)(ulAddr & 0xff);
        }

        ///
        /// \brief A "blank" IP address value.
        ///     This value should be used sparingly and only where required.
        ///     I.e. when the HNAP specification calls for an empty value.
        ///
        ///     \retval a "blank" IP address value.
        ///
        static const IPv4Address & Blank() throw()
        {
            static IPv4Address s_ipaddrBlank((const HDK_XML_IPAddress*)0);

            return s_ipaddrBlank;
        }

        ///
        /// \brief Convert this IPv4 address to an unsigned 32-bit value.
        ///      \note If the IP address is blank, this will return 0.
        ///
        ///      \retval This IP address as an unsigned 32-bit value.
        ///
        unsigned long ToULong() const throw()
        {
            return ((unsigned long)m_hdkipaddr.a << 24) | ((unsigned long)m_hdkipaddr.b << 16) |
                ((unsigned long)m_hdkipaddr.c << 8) | (unsigned long)m_hdkipaddr.b;
        }

        IPv4Address & operator=(const IPv4Address & rhs) throw()
        {
            if (this != &rhs)
            {
                m_fBlank = rhs.m_fBlank;
                m_hdkipaddr = rhs.m_hdkipaddr;
            }

            return *this;
        }

        bool operator==(const IPv4Address & rhs) const throw()
        {
            return (m_fBlank == rhs.m_fBlank) &&
                   (0 == memcmp(&m_hdkipaddr, &rhs.m_hdkipaddr, sizeof(m_hdkipaddr)));
        }
        bool operator!=(const IPv4Address & rhs) const throw()
        {
            return !operator==(rhs);
        }

        ///
        /// \brief Convert this IPv4 address to a string in dot-decimal notation.
        ///      This method returns this IPv4 address as a string in dot-decimal notation (e.g. 192.168.0.1)
        ///      The caller is responsible for suppling a buffer in which to copy the string value.
        ///
        ///      \arg psz A buffer to store the string.
        ///
        char* ToString(char psz[16]) const throw()
        {
            if (!IsBlank())
            {
                HDK_XML_OutputStream_BufferContext bufferCtx;
                bufferCtx.pBuf = psz;
                bufferCtx.cbBuf = 16;
                bufferCtx.ixCur = 0;

                return HDK_XML_Serialize_IPAddress(0, HDK_XML_OutputStream_Buffer, &bufferCtx, &m_hdkipaddr, 1) ? psz : 0;
            }
            else
            {
                *psz = '\0';
                return psz;
            }
        }

#if defined(HDK_CLI_CPP_USE_STL)
        std::string ToString() const
        {
            char szTmp[16];
            return ToString(szTmp) ? std::string(szTmp) : std::string();
        }
#endif

        ///
        ///
        /// \brief Set the IPv4 address from the given string in dot-decimal notation.
        ///      This method sets the IPv4 address from a caller-supplied string in dot-decimal notation
        ///      (e.g. 192.168.0.1).  Invalid strings do not affect the IPv4 address value.
        ///      \note Leading and trailing whitespace is allowed.
        ///      \note The empty string or a string of only whitespace is treated as 0.0.0.0
        ///
        ///      \retval true if the given string was valid, false if not.
        ///
        bool FromString(const char* pszAddr) throw()
        {
            bool fParsed = pszAddr && (0 != HDK_XML_Parse_IPAddress(&m_hdkipaddr, pszAddr));
            if (fParsed)
            {
                m_fBlank = false;
            }

            return fParsed;
        }

#if defined(HDK_CLI_USE_STL)
        bool FromString(const std::string& str)
        {
            return FromString(str.c_str());
        }
#endif

        operator const HDK_XML_IPAddress*() const throw()
        {
            return &m_hdkipaddr;
        }

        bool IsBlank() const throw()
        {
            return m_fBlank;
        }
    private:

        //
        // Members.
        //
        HDK_XML_IPAddress m_hdkipaddr;
        bool m_fBlank;
    }; // class IPv4Address

    ///
    /// \class MAC address type.
    ///     This object should be used when dealing with MAC addresses in the HDK.
    ///
    class MACAddress
    {
    public:
        //
        // Constructors/Destructor.
        //
        MACAddress() throw() :
            m_hdkmacaddr(),
            m_fBlank(true)
        {
            memset(&m_hdkmacaddr, 0, sizeof(m_hdkmacaddr));
        }
        MACAddress(const HDK_XML_MACAddress* phdkmacaddr) throw() :
            m_hdkmacaddr(),
            m_fBlank(0 == phdkmacaddr)
        {
            if (0 != phdkmacaddr)
            {
                m_hdkmacaddr = *phdkmacaddr;
            }
            else
            {
                memset(&m_hdkmacaddr, 0, sizeof(m_hdkmacaddr));
            }
        }
        MACAddress(const MACAddress & src) throw() :
            m_hdkmacaddr(src.m_hdkmacaddr),
            m_fBlank(src.m_fBlank)
        {
        }
        MACAddress(const HDK_XML_MACAddress & hdkmacaddr) throw() :
            m_hdkmacaddr(),
            m_fBlank(false)
        {
            m_hdkmacaddr = hdkmacaddr;
        }
        MACAddress(const unsigned char* pbAddr) throw() :
            m_hdkmacaddr(),
            m_fBlank(false)
        {
            memcpy(&m_hdkmacaddr, pbAddr, sizeof(m_hdkmacaddr));
        }

        MACAddress & operator=(const MACAddress & rhs) throw()
        {
            if (this != &rhs)
            {
                m_fBlank = rhs.m_fBlank;
                m_hdkmacaddr = rhs.m_hdkmacaddr;
            }

            return *this;
        }

        ///
        /// \brief A "blank" MAC address value.
        ///     This value should be used sparingly and only where required.
        ///     I.e. when the HNAP specification calls for an empty value.
        ///
        ///     \retval a "blank" MAC address value.
        ///
        static const MACAddress & Blank() throw()
        {
            static MACAddress s_macaddrBlank((const HDK_XML_MACAddress*)(0));

            return s_macaddrBlank;
        }

        ///
        /// \brief Return the raw bytes of this MAC address.
        ///     \note If the MAC address is blank, this will return 0.
        ///
        ///     \retval The raw bytes.
        ///
        const unsigned char* ToBytes() const throw()
        {
            if (IsBlank())
            {
                return 0;
            }
            else
            {
                return (const unsigned char*)&m_hdkmacaddr;
            }
        }

        bool operator==(const MACAddress & rhs) const throw()
        {
            return (m_fBlank == rhs.m_fBlank) &&
                   (0 == memcmp(&m_hdkmacaddr, &rhs.m_hdkmacaddr, sizeof(m_hdkmacaddr)));
        }
        bool operator!=(const MACAddress & rhs) const throw()
        {
            return !operator==(rhs);
        }

        ///
        /// \brief Convert this MAC address to a string in colon-byte notation.
        ///     This method returns this MAC address as a string in colon-byte notation (e.g. 00:11:22:33:AA:BB:CC)
        ///     The caller is responsible for suppling a buffer in which to copy the string value.
        ///
        ///     \arg psz A buffer to store the string.
        ///     \retval A pointer to the stored string or 0 on failure.
        ///
        char* ToString(char psz[18]) const throw()
        {
            if (!IsBlank())
            {
                HDK_XML_OutputStream_BufferContext bufferCtx;
                bufferCtx.pBuf = psz;
                bufferCtx.cbBuf = 18;
                bufferCtx.ixCur = 0;

                return HDK_XML_Serialize_MACAddress(0, HDK_XML_OutputStream_Buffer, &bufferCtx, &m_hdkmacaddr, 1) ? psz : 0;
            }
            else
            {
                *psz = '\0';
                return psz;
            }
        }

#if defined(HDK_CLI_CPP_USE_STL)
        std::string ToString() const
        {
            char szTmp[18];
            return ToString(szTmp) ? std::string(szTmp) : std::string();
        }
#endif

        ///
        /// \brief Set the MAC address from the given string in colon-byte notation.
        ///     This method sets the MAC address from a caller-supplied string in colon-byte notation
        ///     (e.g. 00:11:22:33:AA:BB:CC).  Invalid strings do not affect the MAC address value.
        ///      \note Leading and trailing whitespace is allowed.
        ///      \note The empty string or a string of only whitespace is treated as 00:00:00:00:00:00.
        ///
        ///     \retval true if the given string was valid, false if not.
        ///
        bool FromString(const char* pszAddr) throw()
        {
            bool fParsed = pszAddr && (0 != HDK_XML_Parse_MACAddress(&m_hdkmacaddr, pszAddr));
            if (fParsed)
            {
                m_fBlank = false;
            }

            return fParsed;
        }

#if defined(HDK_CLI_USE_STL)
        bool FromString(const std::string& str)
        {
            return FromString(str.c_str());
        }
#endif

        operator const HDK_XML_MACAddress*() const throw()
        {
            return &m_hdkmacaddr;
        }

        bool IsBlank() const throw()
        {
            return m_fBlank;
        }
    private:

        //
        // Members.
        //
        HDK_XML_MACAddress m_hdkmacaddr;
        bool m_fBlank;
    }; // class MACAddress

    ///
    /// \class Blob type.
    ///     This object should be used when dealing with blobs in the HDK.
    ///
    class Blob
    {
    public:
        //
        // Constructors/Destructor.
        //
        Blob() throw() :
            m_pb(0),
            m_cb(0)
        {
        }
        Blob(const Blob & src) throw() :
            m_pb(0),
            m_cb(0)
        {
            m_pb = (char*)malloc(src.m_cb);
            if (m_pb)
            {
                m_cb = src.m_cb;

                memcpy(m_pb, src.m_pb, m_cb);
            }
        }
        Blob(const char* pb, unsigned int cb) throw() :
            m_pb(0),
            m_cb(0)
        {
            m_pb = (char*)malloc(cb);
            if (m_pb)
            {
                m_cb = cb;

                memcpy(m_pb, pb, m_cb);
            }
        }
        ~Blob() throw()
        {
            Release();
        }

        Blob & operator=(const Blob & rhs) throw()
        {
            if (this != &rhs)
            {
                char* pb = (char*)realloc(m_pb, rhs.m_cb);
                if (!pb)
                {
                    free(m_pb);
                }
                m_pb = pb;
                if (m_pb)
                {
                    m_cb = rhs.m_cb;
                    memcpy(m_pb, rhs.m_pb, m_cb);
                }
                else
                {
                    m_cb = 0;
                }
            }

            return *this;
        }

        bool operator==(const Blob & rhs) const throw()
        {
            return (m_cb == rhs.m_cb) &&
                   (0 == memcmp(m_pb, rhs.m_pb, m_cb));
        }
        bool operator!=(const Blob & rhs) const throw()
        {
            return !operator==(rhs);
        }

        const char* get_Data() const throw()
        {
            return m_pb;
        }
        unsigned int get_Size() const throw()
        {
            return m_cb;
        }

        operator const char*() const throw()
        {
            return get_Data();
        }

    private:
        void Release() throw()
        {
            free(m_pb);
            m_cb = 0;
        }
        //
        // Members.
        //
        char* m_pb;
        unsigned int m_cb;
    }; // class Blob

    class UUID
    {
    public:
        //
        // Constructors/Destructor.
        //
        UUID() throw() :
            m_hdkuuid()
        {
            memset(&m_hdkuuid, 0, sizeof(m_hdkuuid));
        }
        UUID(const HDK_XML_UUID* phdkuuid) throw() :
            m_hdkuuid()
        {
            if (0 != phdkuuid)
            {
                memcpy(m_hdkuuid.bytes, phdkuuid->bytes, sizeof(m_hdkuuid.bytes));
            }
            else
            {
                memset(&m_hdkuuid, 0, sizeof(m_hdkuuid));
            }
        }
        UUID(const UUID & src) throw() :
            m_hdkuuid()
        {
            memcpy(m_hdkuuid.bytes, src.m_hdkuuid.bytes, sizeof(m_hdkuuid.bytes));
        }

        UUID(const HDK_XML_UUID & hdkuuid) throw() :
            m_hdkuuid()
        {
            memcpy(m_hdkuuid.bytes, hdkuuid.bytes, sizeof(m_hdkuuid.bytes));
        }

        UUID(const unsigned char bytes[16]) throw() :
            m_hdkuuid()
        {
            memcpy(m_hdkuuid.bytes, bytes, sizeof(m_hdkuuid.bytes));
        }

        UUID & operator=(const UUID & rhs) throw()
        {
            if (this != &rhs)
            {
                memcpy(m_hdkuuid.bytes, rhs.m_hdkuuid.bytes, sizeof(m_hdkuuid.bytes));
            }

            return *this;
        }

        bool operator==(const UUID & rhs) const throw()
        {
            return (0 == memcmp(m_hdkuuid.bytes, rhs.m_hdkuuid.bytes, sizeof(m_hdkuuid.bytes)));
        }
        bool operator!=(const UUID & rhs) const throw()
        {
            return !operator==(rhs);
        }

        bool IsZero() const throw()
        {
            for (size_t ix = 0; ix < sizeof(m_hdkuuid.bytes); ix++)
            {
                if (m_hdkuuid.bytes[ix])
                {
                    return false;
                }
            }

            return true;
        }

        static UUID Zero()
        {
            return UUID();
        }

        ///
        /// \brief Convert this UUID to a string in the format \x{8}-\x{4}-\x{4}-\x{4}-\x{12}.
        ///      This method returns this UUID as a string.
        ///      The caller is responsible for suppling a buffer in which to copy the string value.
        ///
        ///      \arg psz A buffer to store the string.
        ///
        char* ToString(char psz[37]) const throw()
        {
            HDK_XML_OutputStream_BufferContext bufferCtx;
            bufferCtx.pBuf = psz;
            bufferCtx.cbBuf = 37;
            bufferCtx.ixCur = 0;

            return HDK_XML_Serialize_UUID(0, HDK_XML_OutputStream_Buffer, &bufferCtx, &m_hdkuuid, 1) ? psz : 0;
        }

#if defined(HDK_CLI_CPP_USE_STL)
        std::string ToString() const
        {
            char szTmp[37];
            return ToString(szTmp) ? std::string(szTmp) : std::string();
        }
#endif

        ///
        ///
        /// \brief Set the UUID from the given string in the format \x{8}-\x{4}-\x{4}-\x{4}-\x{12}.
        ///      This method sets the UUID from a caller-supplied string.  Invalid strings do
        ///      not affect the UUID value.
        ///      \note Leading and trailing whitespace is allowed.
        ///      \note The empty string or a string of only whitespace is treated as 00000000-00000-00000-000000000000
        ///
        ///      \retval true if the given string was valid, false if not.
        ///
        bool FromString(const char* pszUUID) throw()
        {
            return pszUUID && (0 != HDK_XML_Parse_UUID(&m_hdkuuid, pszUUID));
        }

#if defined(HDK_CLI_USE_STL)
        bool FromString(const std::string& str)
        {
            return FromString(str.c_str());
        }
#endif

        operator const HDK_XML_UUID*() const throw()
        {
            return &m_hdkuuid;
        }

    private:
        //
        // Members.
        //
        HDK_XML_UUID m_hdkuuid;
    }; // class UUID

    class Struct
    {
    public:
        enum SerializeOptions
        {
            NoNewLines = HDK_XML_SerializeOption_NoNewlines  /* don't output newlines */
        };
    public:
        //
        // Constructors/Destructor.
        //

        ///
        /// \brief Create a new struct object.
        ///
        Struct() throw() :
            m_phdkstruct(0),
            m_fOwned(true)
        {
            // Allocate a new struct.
            m_phdkstruct = (HDK_XML_Struct*)malloc(sizeof(HDK_XML_Struct));
            if (m_phdkstruct)
            {
                HDK_XML_Struct_Init(m_phdkstruct);
                m_phdkstruct->node.element = HDK_XML_BuiltinElement_Unknown;
            }
        }

        ///
        /// \brief Create a new struct for the given element.
        ///
        Struct(HDK_XML_Element element) throw() :
            m_phdkstruct(0),
            m_fOwned(true)
        {
            // Allocate a new struct.
            m_phdkstruct = (HDK_XML_Struct*)malloc(sizeof(HDK_XML_Struct));
            if (m_phdkstruct)
            {
                HDK_XML_Struct_Init(m_phdkstruct);
                m_phdkstruct->node.element = element;
            }
        }

        ///
        /// \brief Wrap the given struct for object-like accessors.
        ///
        Struct(HDK_XML_Struct* phdkstruct) throw() :
            m_phdkstruct(phdkstruct),
            m_fOwned(false)
        {
        }

        ///
        /// \brief Construct a new Struct object making a 'deep' copy of the given struct.
        ///
        Struct(const Struct & src) throw() :
            m_phdkstruct(0),
            m_fOwned(false)
        {
            (void)operator=(src);
        }

        virtual ~Struct() throw()
        {
            Release();
        }

        static const Struct & Null() throw()
        {
            static Struct s_structNull((HDK_XML_Struct*)(0));

            return s_structNull;
        }

        ///
        /// \brief Perform a 'deep' copy of a given struct into this Struct object.
        ///
        Struct & operator=(const Struct & rhs) throw()
        {
            if (this != &rhs)
            {
                Release();

                if (!rhs.IsNull())
                {
                    // Allocate a new struct.
                    HDK_XML_Struct sTmp;
                    HDK_XML_Struct_Init(&sTmp);

                    // Create a copy and hold onto it.
                    HDK_XML_SetEx_Struct(&sTmp, rhs.m_phdkstruct->node.element, rhs);
                    m_phdkstruct = HDK_XML_Get_Struct(&sTmp, rhs.m_phdkstruct->node.element);

                    // HDK_XML_Struct_Free(&sTmp) is not called so as to not release the copied struct.

                    m_fOwned = true;
                }
            }

            return *this;
        }

        bool IsNull() const throw()
        {
            return (0 == m_phdkstruct);
        }

        HDK_XML_Struct* operator&() throw()
        {
            // If this->IsNull(), 0 would be returned
            // If !m_fOwned, a memory leak could occur in addition to potentially writing memory owned some other code
            // If m_phdkstruct->pHead != 0 serves as a warning as this is not recommended use. The &operator should
            // only be used for receiving output from a request or XML parse operation.
            assert(!IsNull() && m_fOwned && (0 == m_phdkstruct->pHead));
            return GetStruct();
        }

        HDK_XML_Element Element() const throw()
        {
            if (!IsNull())
            {
                return m_phdkstruct->node.element;
            }

            return HDK_XML_BuiltinElement_Unknown;
        }

        operator const HDK_XML_Struct*() const throw()
        {
            return m_phdkstruct;
        }

        bool SerializeToFile(const HDK_XML_Schema* pSchema, const char* pszFile, int options) const throw()
        {
            FILE* pfh = fopen(pszFile, "wb");
            if (pfh)
            {
                bool fResult = SerializeToFile(pSchema, pfh, options);
                fclose(pfh);

                return fResult;
            }

            return false;
        }

        bool SerializeToFile(const HDK_XML_Schema* pSchema, FILE* pfh, int options) const throw()
        {
            unsigned int cbStream = 0;
            return (0 != HDK_XML_Serialize(&cbStream, HDK_XML_OutputStream_File, pfh, pSchema, *this, options));
        }

        bool DeserializeFromFile(const HDK_XML_Schema* pSchema, const char* pszFile) throw()
        {
            FILE* pfh = fopen(pszFile, "rb");
            if (pfh)
            {
                bool fResult = DeserializeFromFile(pSchema, pfh);
                fclose(pfh);

                return fResult;
            }

            return false;
        }

        bool DeserializeFromFile(const HDK_XML_Schema* pSchema, FILE* pfh) throw()
        {
            return (HDK_XML_ParseError_OK == HDK_XML_Parse(HDK_XML_InputStream_File, pfh, pSchema, &(*this), 0));
        }

        bool Validate(const HDK_XML_Schema* pSchema) const throw()
        {
            return (0 != HDK_XML_Validate(pSchema, *this, 0));
        }

    protected:

        HDK_XML_Struct* GetStruct() const throw()
        {
            return m_phdkstruct;
        }

        bool IsOwned() const throw()
        {
            return m_fOwned;
        }

    private:
        void Release() throw()
        {
            if (m_fOwned && (0 != m_phdkstruct))
            {
                HDK_XML_Struct_Free(m_phdkstruct);
                free(m_phdkstruct);
            }
            m_fOwned = false;
            m_phdkstruct = 0;
        }

        //
        // Members.
        //
        HDK_XML_Struct* m_phdkstruct;
        bool m_fOwned;
    }; // class Struct

    /*!
      \interface Target exposes a method to send an HDK request to a generic target.

      The target-specific semantics should be encapsulated by the implementor of a Target object.
      E.g. a IP-connected device target would implement request by sending an HNAP request over TCP/IP.
      The target's IP would encapsulated by the Target object.
     */
    class ITarget
    {
    public:
        virtual ~ITarget()
        {
        }

        /*!
          \brief Call a given HDK method on this target.

          This method must be implemented by all HDK Targets.

          \param[in] timeoutSecs - The timeout for the request.  0 indicates a reasonable default should be used.
          \param[in] pModule - The HDK module pointer, which containes the method
          \param[in] method - The method enumeration value.
          \param[in] pInput - The input parameters for the HNAP call.
          \param[out] pOutput - The response output from the HDK method call.
         */
        virtual ClientError Request
        (
            unsigned int timeoutSecs,
            const HDK_MOD_Module* pModule,
            int method,
            const HDK_XML_Struct* pInput,
            HDK_XML_Struct* pOutput
        ) const throw() = 0;
    }; // class ITarget

    class HNAPDevice : public ITarget
    {
    public:

        HNAPDevice() throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(0),
            m_fUseSsl(false)
        {
        }
        HNAPDevice(const char* pszHost) throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(80),
            m_fUseSsl(false)
        {
            Host(pszHost);
        }
        HNAPDevice(const char* pszHost, const char* pszUsername, const char* pszPassword) throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(80),
            m_fUseSsl(false)
        {
            Host(pszHost);
            Username(pszUsername);
            Password(pszPassword);
        }
        HNAPDevice(const char* pszHost, unsigned short port, const char* pszUsername, const char* pszPassword) throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(port),
            m_fUseSsl(false)
        {
            Host(pszHost);
            Username(pszUsername);
            Password(pszPassword);
        }
        HNAPDevice(const IPv4Address & ip, unsigned short port) throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(port),
            m_fUseSsl(false)
        {
            Host(ip);
        }
        HNAPDevice(const IPv4Address & ip, unsigned short port, const char* pszUsername, const char* pszPassword) throw() :
            m_pszUsername(0),
            m_pszPassword(0),
            m_pszHost(0),
            m_port(port),
            m_fUseSsl(false)
        {
            Host(ip);
            Username(pszUsername);
            Password(pszPassword);
        }

        virtual ~HNAPDevice() throw()
        {
            free(m_pszUsername);
            free(m_pszPassword);
            free(m_pszHost);
        }

        ///
        /// \brief Make a request to this device
        ///
        ///     \param[in] timeoutSecs The timeout for the request.
        ///     \param[in] pModule The module in which the method is defined.
        ///     \param[in] method The method number (index into the module's method list)
        ///     \param[in] pInput The input parameters for the request.
        ///     \param[out] pOuput The returned output from the request.
        ///     \retval The error code for the request.
        ///
        ClientError Request
        (
            unsigned int timeoutSecs,           /* The request timeout, in seconds. 0 indicates the system default. */
            const HDK_MOD_Module* pModule,      /* The HDK module pointer */
            int method,                         /* The method enumeration value */
            const HDK_XML_Struct* pInput,       /* The input parameters for the HNAP call. */
            HDK_XML_Struct* pOutput
        ) const throw()
        {
            char* pszUrl = HttpUrl();
            if (!pszUrl)
            {
                return ClientError_OutOfMemory;
            }

            HDK_CLI_Error hdkerror = HDK_CLI_Request(0,
                                                     pszUrl,
                                                     Username(),
                                                     Password(),
                                                     timeoutSecs,
                                                     pModule,
                                                     method,
                                                     pInput,
                                                     pOutput);
            free(pszUrl);

            return (ClientError)hdkerror;
        }

        const char* Username() const throw()
        {
            return m_pszUsername;
        }
        bool Username(const char* pszUsername) throw()
        {
            return AllocateAndCopyString(&m_pszUsername, pszUsername);
        }

        const char* Password() const throw()
        {
            return m_pszPassword;
        }
        bool Password(const char* pszPassword) throw()
        {
            return AllocateAndCopyString(&m_pszPassword, pszPassword);
        }

        const char* Host() const throw()
        {
            return m_pszHost;
        }
        bool Host(const char* pszHost) throw()
        {
            return AllocateAndCopyString(&m_pszHost, pszHost);
        }
        bool Host(const IPv4Address & ip) throw()
        {
            char* pszHost = (char*)realloc(m_pszHost, sizeof("255.255.255.255"));
            if (!pszHost)
            {
                free(m_pszHost);
            }
            m_pszHost = pszHost;
            if (m_pszHost)
            {
                return (0 != ip.ToString(m_pszHost));
            }

            return false;
        }

        unsigned short Port() const throw()
        {
            return m_port;
        }
        void Port(unsigned short port) throw()
        {
            m_port = port;
        }

        bool UseSsl() const throw()
        {
            return m_fUseSsl;
        }
        void UseSsl(bool fUseSsl) throw()
        {
            m_fUseSsl = fUseSsl;
        }

    private:
        HNAPDevice(const HNAPDevice &) throw(); // no copy constructor
        HNAPDevice & operator=(const HNAPDevice &) throw(); // no = operator

        // Caller must free() returned string
        char* HttpUrl() const throw()
        {
            size_t cbUrl = (m_pszHost ? strlen(m_pszHost) : 0) + sizeof("https://:65535");
            char* pszUrl = (char*)malloc(cbUrl);
            if (pszUrl)
            {
#ifdef _MSC_VER
                sprintf_s(pszUrl, cbUrl, "http%s://%s:%u", m_fUseSsl ? "s" : "",
                                                           m_pszHost ? m_pszHost : "",
                                                           m_port);
#else // ndef _MSC_VER
                sprintf(pszUrl, "http%s://%s:%u", m_fUseSsl ? "s" : "",
                                                  m_pszHost ? m_pszHost : "",
                                                  m_port);
#endif // def _MSC_VER
            }

            return pszUrl;
        }

        static bool AllocateAndCopyString(char** ppszDst, const char* pszSrc) throw()
        {
            if (pszSrc)
            {
                size_t cbDst = strlen(pszSrc) + 1 /* for '\0' */;
                char* pszDst = (char*)realloc(*ppszDst, cbDst);
                if (!pszDst)
                {
                    free(*ppszDst);
                }
                *ppszDst = pszDst;
                if (*ppszDst)
                {
#ifdef _MSC_VER
                    strcpy_s(*ppszDst, cbDst, pszSrc);
#else // ndef _MSC_VER
                    strcpy(*ppszDst, pszSrc);
#endif // def _MSC_VER
                }

                return (0 != *ppszDst);
            }
            else
            {
                free(*ppszDst);
                *ppszDst = 0;

                return true;
            }
        }

        //
        // Members.
        //
        char* m_pszUsername;
        char* m_pszPassword;

        char* m_pszHost;
        unsigned short m_port;
        bool m_fUseSsl;
    }; // class HNAPDevice : public ITarget

    ///
    /// \brief Array type wrapper base class
    ///
    class Array : public Struct
    {
    public:

        ///
        /// \class ArrayIter
        ///      Iterator base class for iterating through the elements of an Array.
        ///
        class ArrayIter
        {
        public:
            ArrayIter & operator=(const ArrayIter & rhs) throw()
            {
                m_pMemberCurrent = rhs.m_pMemberCurrent;
                return *this;
            }

            bool operator==(const Array::ArrayIter & rhs) const throw()
            {
                return (m_pMemberCurrent == rhs.m_pMemberCurrent);
            }
            bool operator!=(const Array::ArrayIter & rhs) const throw()
            {
                return !operator==(rhs);
            }

            ArrayIter & operator++() throw()
            {
                /*
                 * If you see an access violation because m_phdkmember is 0, you are
                 * trying to iterate pass the end of the iterator.
                 */
                m_pMemberCurrent = m_pMemberCurrent->pNext;
                return *this;
            }
            ArrayIter operator++(int) throw()
            {
                // Increment this, but return the original value.
                HDK_XML_Member* pMemberCurrent = m_pMemberCurrent;
                m_pMemberCurrent = m_pMemberCurrent->pNext;
                return ArrayIter(pMemberCurrent);
            }

        protected:
            ArrayIter(HDK_XML_Member* pMemberHead) throw() :
                m_pMemberCurrent(pMemberHead)
            {
            }
            // Copy constructor use is protected
            ArrayIter(const ArrayIter & src) throw() :
                m_pMemberCurrent(src.m_pMemberCurrent)
            {
            }

            HDK_XML_Member* GetMember() throw()
            {
                return m_pMemberCurrent;
            }
        private:
            ArrayIter() throw(); // no default constructor

            //
            // Members.
            //
            HDK_XML_Member* m_pMemberCurrent;
        }; // class ArrayIter

        ///
        /// \brief Get the size of the array.
        ///      \retval The number of elements in the array.
        ///
        size_t size() const throw()
        {
            size_t nLength = 0;
            for (HDK_XML_Member* pMember = GetStruct() ? GetStruct()->pHead : 0; 0 != pMember; pMember = pMember->pNext)
            {
                nLength++;
            }

            return nLength;
        }

        ///
        /// \brief Is the array empty?
        ///
        bool empty() const throw()
        {
            return !GetStruct() || !GetStruct()->pHead;
        }
    protected:

        //
        // Constructors/Destructor.
        //
        Array() throw() :
            Struct()
        {
        }
        Array(HDK_XML_Struct* pStruct) throw() :
            Struct(pStruct)
        {
        }
        Array(HDK_XML_Element element) throw() :
            Struct(element)
        {
        }
        Array(const Array & src) throw() :
            Struct(src)
        {
        }
    }; // class Array

    ///
    /// \brief Templatized array wrapper for struct types
    ///    _StructType : The c++ class wrapper for the struct type.
    ///    _element : The hdk-defined schema element for the array members.
    ///
    template<class _StructType, int _element> class StructArray : public Array
    {
    public:
        class StructArrayIter : public ArrayIter
        {
        friend class StructArray;
        public:
            _StructType operator*() throw()
            {
                return value();
            }

            _StructType value() throw()
            {
                return HDK_XML_GetMember_Struct(GetMember());
            }
        protected:
            StructArrayIter(HDK_XML_Member* pMember) throw() :
                ArrayIter(pMember)
            {
            }
        private:
            StructArrayIter() throw(); // no default constructor
        }; // class StructArrayIter : public ArrayIter

        StructArray() throw() :
            Array()
        {
        }
        StructArray(HDK_XML_Element element) throw() :
            Array(element)
        {
        }
        StructArray(HDK_XML_Struct* pStruct) throw() :
            Array(pStruct)
        {
        }

        StructArrayIter begin() const throw()
        {
            return StructArrayIter(GetStruct() ? GetStruct()->pHead : 0);
        }
        StructArrayIter end() const throw()
        {
            return StructArrayIter(0);
        }

        bool append(const _StructType & value) throw()
        {
            return (0 != HDK_XML_AppendEx_Struct(GetStruct(), _element, value));
        }
    }; // template<class _StructType, int _element> class StructArray : public Array

    ///
    /// \brief Templatized array wrapper for enum types
    ///    _EnumType : The c++ enum type.
    ///    _xmlType : The hdk type (HDK_XML_Type) representing this enum.
    ///    _element : The hdk-defined schema element for the array members.
    ///
    template<typename _EnumType, int _xmlType, int _element> class EnumArray : public Array
    {
    public:
        class EnumArrayIter : public ArrayIter
        {
        friend class EnumArray;
        public:
            _EnumType operator*() throw()
            {
                return value();
            }

            _EnumType value() throw()
            {
                return (_EnumType)*HDK_XML_GetMember_Enum(GetMember(), _xmlType);
            }
        protected:
            EnumArrayIter(HDK_XML_Member* pMember) throw() :
                ArrayIter(pMember)
            {
            }
        private:
            EnumArrayIter() throw(); // no default constructor
        }; // class EnumArrayIter : public ArrayIter

        EnumArray() throw() :
            Array()
        {
        }
        EnumArray(HDK_XML_Element element) throw() :
            Array(element)
        {
        }
        EnumArray(HDK_XML_Struct* pStruct) throw() :
            Array(pStruct)
        {
        }

        EnumArrayIter begin() const throw()
        {
            return EnumArrayIter(GetStruct() ? GetStruct()->pHead : 0);
        }
        EnumArrayIter end() const throw()
        {
            return EnumArrayIter(0);
        }

        bool append(_EnumType value) throw()
        {
            return (0 != HDK_XML_Append_Enum(GetStruct(), _element, _xmlType, (int)value));
        }
    }; // template<typename _EnumType, typename _HDKType, int _element> class EnumArray : public Array

    ///
    /// \brief Templatized array wrapper for blob type.
    ///    _element : The hdk-defined schema element for the array members.
    ///
    template<int _element> class BlobArray : public Array
    {
    public:
        class BlobArrayIter : public ArrayIter
        {
        friend class BlobArray;
        public:
            Blob operator*() throw()
            {
                return value();
            }

            Blob value() throw()
            {
                unsigned int cb = 0;
                char* pb = HDK_XML_GetMember_Blob(GetMember(), &cb);
                return Blob(pb, cb);
            }
        protected:
            BlobArrayIter(HDK_XML_Member* pMember) throw() :
                ArrayIter(pMember)
            {
            }
        private:
            BlobArrayIter() throw(); // no default constructor
        }; // class BlobArrayIter : public ArrayIter

        BlobArray() throw() :
            Array()
        {
        }
        BlobArray(HDK_XML_Element element) throw() :
            Array(element)
        {
        }
        BlobArray(HDK_XML_Struct* pStruct) throw() :
            Array(pStruct)
        {
        }

        BlobArrayIter begin() const throw()
        {
            return BlobArrayIter(GetStruct() ? GetStruct()->pHead : 0);
        }
        BlobArrayIter end() const throw()
        {
            return BlobArrayIter(0);
        }

        bool append(const Blob & value) throw()
        {
            return (0 != HDK_XML_Append_Blob(GetStruct(), _element, value.get_Data(), value.get_Size()));
        }
    }; // template<int _element> class BlobArray : public Array

#define DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS(_hdkType, _cppType) \
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(_hdkType, _cppType, *, _cppType)

#define DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(_hdkType, _cppType, _GetMemberCast, _appendType) \
    template<int _element> class _hdkType ## Array : public Array \
    { \
    public: \
        class _hdkType ## ArrayIter : public ArrayIter \
        { \
        friend class _hdkType ## Array; \
        public: \
            _cppType operator*() throw() \
            { \
                return value(); \
            } \
            _cppType value() throw() \
            { \
                return _GetMemberCast(HDK_XML_GetMember_ ## _hdkType(GetMember())); \
            } \
        protected: \
            _hdkType ## ArrayIter(HDK_XML_Member* pMember) throw() : \
                ArrayIter(pMember) \
            { \
            } \
        private: \
            _hdkType ## ArrayIter(); \
        }; \
        _hdkType ## Array() throw() : \
            Array() \
        { \
        } \
        _hdkType ## Array(HDK_XML_Element element) throw() : \
            Array(element) \
        { \
        } \
        _hdkType ## Array(HDK_XML_Struct* pStruct) throw() : \
            Array(pStruct) \
        { \
        } \
        _hdkType ## ArrayIter begin() const throw() \
        { \
            return _hdkType ## ArrayIter(GetStruct() ? GetStruct()->pHead : 0); \
        } \
        _hdkType ## ArrayIter end() const throw() \
        { \
            return _hdkType ## ArrayIter(0); \
        } \
        bool append (_appendType value) throw() \
        { \
            return (0 != HDK_XML_Append_ ## _hdkType (GetStruct(), _element, value)); \
        } \
    };

    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(Bool, bool, 0 != *, bool)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS(DateTime, time_t)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS(Int, HDK_XML_Int)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS(Long, HDK_XML_Long)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(String, const char*, (const char*), const char*)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(IPAddress, IPv4Address, *, const IPv4Address &)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(MACAddress, MACAddress, *, const MACAddress &)
    DEFINE_HDK_BUILTIN_TYPE_ARRAY_CLASS_EX(UUID, UUID, *, const UUID &)

} // namespace HDK
