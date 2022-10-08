/** @addtogroup MC_RTM
 * @{
 * MobiCore Version Information
 *
 * <!-- Copyright Giesecke & Devrient GmbH 2009-2012 -->
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MCVERSIONINFO_H_
#define MCVERSIONINFO_H_

/** Length of MobiCore product ID string. */
#define MC_PRODUCT_ID_LEN 64

/** Global MobiCore Version Information.
 */
typedef struct {
    char productId[MC_PRODUCT_ID_LEN]; /** < Product ID of Mobicore; zero-terminated */
    uint32_t versionMci;               /** < Version of Mobicore Control Interface */
    uint32_t versionSo;                /** < Version of Secure Objects */
    uint32_t versionMclf;              /** < Version of MobiCore Load Format */
    uint32_t versionContainer;         /** < Version of MobiCore Container Format */
    uint32_t versionMcConfig;          /** < Version of MobiCore Configuration Block Format */
    uint32_t versionTlApi;             /** < Version of MobiCore Trustlet API Implementation */
    uint32_t versionDrApi;             /** < Version of MobiCore Driver API Implementation */
    uint32_t versionCmp;               /** < Version of Content Management Protocol */
} mcVersionInfo_t;

#endif /** MCVERSIONINFO_H_ */
