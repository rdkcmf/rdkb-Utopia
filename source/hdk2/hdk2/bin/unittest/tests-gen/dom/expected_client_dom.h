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

// actual_client_dom.h - [Generated by hdkcli_cpp]

// Non-generated client code.
#include "hdk_cli_cpp.h"

// Underlying schema module.
#include "actual_client_dom_mod.h"


namespace HDK
{

    namespace Cisco
    {
        ///
        /// \brief CiscoStruct
        ///      <a>http://cisco.com/HNAPExt/CiscoStruct</a>
        ///
        class CiscoStructStruct : public Struct
        {
        public:
            //
            // Constructors/Destructor.
            //
            CiscoStructStruct() throw();

            CiscoStructStruct(HDK_XML_Struct* phdkstruct) throw();

            ///
            /// \brief Get the a value.
            ///
            HDK_XML_Int get_a() const throw();

            ///
            /// \brief Set the a value.
            ///
            void set_a(HDK_XML_Int value) throw();

            ///
            /// \brief Get the b value.
            ///
            const char* get_b() const throw();

            ///
            /// \brief Set the b value.
            ///
            void set_b(const char* value) throw();

            ///
            /// \brief Get the c value.
            ///
            HDK_XML_Long get_c() const throw();

            ///
            /// \brief Set the c value.
            ///
            void set_c(HDK_XML_Long value) throw();

            ///
            /// \brief Serialize to/from an XML file.
            ///
            bool FromFile(const char* pszFile) throw();
            bool ToFile(const char* pszFile) const throw();

        }; // class CiscoStructStruct : public Struct

        ///
        /// \brief CiscoStructToo
        ///      <a>http://cisco.com/HNAPExt/CiscoStructToo</a>
        ///
        class CiscoStructTooStruct : public Struct
        {
        public:
            //
            // Constructors/Destructor.
            //
            CiscoStructTooStruct() throw();

            CiscoStructTooStruct(HDK_XML_Struct* phdkstruct) throw();

            ///
            /// \brief Get the s value.
            ///
            CiscoStructStruct get_s() const throw();

            ///
            /// \brief Set the s value.
            ///
            void set_s(const CiscoStructStruct& value) throw();

            ///
            /// \brief Get the d value.
            ///
            HDK_XML_Int get_d() const throw();

            ///
            /// \brief Set the d value.
            ///
            void set_d(HDK_XML_Int value) throw();

            ///
            /// \brief Get the e value.
            ///
            const char* get_e() const throw();

            ///
            /// \brief Set the e value.
            ///
            void set_e(const char* value) throw();

            ///
            /// \brief Serialize to/from an XML file.
            ///
            bool FromFile(const char* pszFile) throw();
            bool ToFile(const char* pszFile) const throw();

        }; // class CiscoStructTooStruct : public Struct

    } // namespace Cisco
} // namespace HDK
