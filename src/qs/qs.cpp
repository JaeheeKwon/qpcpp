//$file${src::qs::qs.cpp} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//
// Model: qpcpp.qm
// File:  ${src::qs::qs.cpp}
//
// This code has been generated by QM 6.1.1 <www.state-machine.com/qm>.
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #    : LicenseRef-QL-dual
// Issued to    : Any user of the QP/C++ real-time embedded framework
// Framework(s) : qpcpp
// Support ends : 2024-12-31
// License scope:
//
// Copyright (C) 2005 Quantum Leaps, LLC <state-machine.com>.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//
//$endhead${src::qs::qs.cpp} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#define QP_IMPL             // this is QP implementation
#include "qs_port.hpp"      // QS port
#include "qs_pkg.hpp"       // QS package-scope internal interface
#include "qstamp.hpp"       // QP time-stamp
#include "qsafe.h"          // QP Functional Safety (FuSa) Subsystem

// unnamed namespace for local definitions with internal linkage
namespace {
Q_DEFINE_THIS_MODULE("qs")
} // unnamed namespace

//$skip${QP_VERSION} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Check for the minimum required QP version
#if (QP_VERSION < 730U) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8U))
#error qpcpp version 7.3.0 or higher required
#endif
//$endskip${QP_VERSION} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//$define${QS::QS-TX} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QS {

//${QS::QS-TX::initBuf} ......................................................
void initBuf(
    std::uint8_t * const sto,
    std::uint_fast32_t const stoSize) noexcept
{
    priv_.buf      = sto;
    priv_.end      = static_cast<QSCtr>(stoSize);
    priv_.head     = 0U;
    priv_.tail     = 0U;
    priv_.used     = 0U;
    priv_.seq      = 0U;
    priv_.chksum   = 0U;
    priv_.critNest = 0U;

    glbFilter_(-static_cast<enum_t>(QS_ALL_RECORDS));// all global filters OFF
    locFilter_(static_cast<enum_t>(QS_ALL_IDS));     // all local filters ON
    priv_.locFilter_AP = nullptr;                    // deprecated "AP-filter"

    // produce an empty record to "flush" the QS trace buffer
    beginRec_(QS_REC_NUM_(QS_EMPTY));
    endRec_();

    // produce the reset record to inform QSPY of a new session
    target_info_pre_(0xFFU);

    // hold off flushing after successful initialization (see QS_INIT())
}

//${QS::QS-TX::getByte} ......................................................
std::uint16_t getByte() noexcept {
    // NOTE: Must be called IN critical section.
    // Also requires system-level memory access (QF_MEM_SYS()).

    std::uint16_t ret;
    if (priv_.used == 0U) {
        ret = QS_EOD; // set End-Of-Data
    }
    else {
        std::uint8_t const * const buf = priv_.buf; // put in a temporary
        QSCtr tail = priv_.tail; // put in a temporary (register)
        ret = static_cast<std::uint16_t>(buf[tail]); // set the byte to return
        ++tail; // advance the tail
        if (tail == priv_.end) { // tail wrap around?
            tail = 0U;
        }
        priv_.tail = tail;              // update the tail
        priv_.used = (priv_.used - 1U); // one less byte used
    }
    return ret; // return the byte or EOD
}

//${QS::QS-TX::getBlock} .....................................................
std::uint8_t const * getBlock(std::uint16_t * const pNbytes) noexcept {
    // NOTE: Must be called IN critical section.
    // Also requires system-level memory access (QF_MEM_SYS()).

    QSCtr const used = priv_.used; // put in a temporary (register)
    std::uint8_t *buf;

    // any bytes used in the ring buffer?
    if (used != 0U) {
        QSCtr tail      = priv_.tail; // put in a temporary (register)
        QSCtr const end = priv_.end;  // put in a temporary (register)
        QSCtr n = static_cast<QSCtr>(end - tail);
        if (n > used) {
            n = used;
        }
        if (n > static_cast<QSCtr>(*pNbytes)) {
            n = static_cast<QSCtr>(*pNbytes);
        }
        *pNbytes = static_cast<std::uint16_t>(n); // n-bytes available
        buf = priv_.buf;
        buf = &buf[tail]; // the bytes are at the tail

        priv_.used = static_cast<QSCtr>(used - n);
        tail += n;
        if (tail == end) {
            tail = 0U;
        }
        priv_.tail = tail;
    }
    else { // no bytes available
        *pNbytes = 0U;      // no bytes available right now
        buf      = nullptr; // no bytes available right now
    }
    return buf;
}

} // namespace QS
} // namespace QP
//$enddef${QS::QS-TX} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#ifndef QF_MEM_ISOLATE
//$define${QS::filters} vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
namespace QP {
namespace QS {

//${QS::filters::filt_} ......................................................
Filter filt_;

} // namespace QS
} // namespace QP
//$enddef${QS::filters} ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
#endif

//============================================================================
//! @cond INTERNAL

namespace QP {
namespace QS {

//............................................................................
Attr priv_;

//............................................................................
void glbFilter_(std::int_fast16_t const filter) noexcept {
    bool const isRemove = (filter < 0);
    std::uint16_t const rec = isRemove
                  ? static_cast<std::uint16_t>(-filter)
                  : static_cast<std::uint16_t>(filter);
    switch (rec) {
        case QS_ALL_RECORDS: {
            std::uint8_t const tmp = (isRemove ? 0x00U : 0xFFU);
            std::uint_fast8_t i;
            // set all global filters (partially unrolled loop)
            for (i = 0U; i < Q_DIM(filt_.glb); i += 4U) {
                filt_.glb[i     ] = tmp;
                filt_.glb[i + 1U] = tmp;
                filt_.glb[i + 2U] = tmp;
                filt_.glb[i + 3U] = tmp;
            }
            if (isRemove) {
                // leave the "not maskable" filters enabled,
                // see qs.h, Miscellaneous QS records (not maskable)
                //
                filt_.glb[0] = 0x01U;
                filt_.glb[6] = 0x40U;
                filt_.glb[7] = 0xFCU;
                filt_.glb[8] = 0x7FU;
            }
            else {
                // never turn the last 3 records on (0x7D, 0x7E, 0x7F)
                filt_.glb[15] = 0x1FU;
            }
            break;
        }
        case QS_SM_RECORDS:
            if (isRemove) {
                filt_.glb[0] &= static_cast<std::uint8_t>(~0xFEU & 0xFFU);
                filt_.glb[1] &= static_cast<std::uint8_t>(~0x03U & 0xFFU);
                filt_.glb[6] &= static_cast<std::uint8_t>(~0x80U & 0xFFU);
                filt_.glb[7] &= static_cast<std::uint8_t>(~0x03U & 0xFFU);
            }
            else {
                filt_.glb[0] |= 0xFEU;
                filt_.glb[1] |= 0x03U;
                filt_.glb[6] |= 0x80U;
                filt_.glb[7] |= 0x03U;
            }
            break;
        case QS_AO_RECORDS:
            if (isRemove) {
                filt_.glb[1] &= static_cast<std::uint8_t>(~0xFCU & 0xFFU);
                filt_.glb[2] &= static_cast<std::uint8_t>(~0x07U & 0xFFU);
                filt_.glb[5] &= static_cast<std::uint8_t>(~0x20U & 0xFFU);
            }
            else {
                filt_.glb[1] |= 0xFCU;
                filt_.glb[2] |= 0x07U;
                filt_.glb[5] |= 0x20U;
            }
            break;
        case QS_EQ_RECORDS:
            if (isRemove) {
                filt_.glb[2] &= static_cast<std::uint8_t>(~0x78U & 0xFFU);
                filt_.glb[5] &= static_cast<std::uint8_t>(~0x40U & 0xFFU);
            }
            else {
                filt_.glb[2] |= 0x78U;
                filt_.glb[5] |= 0x40U;
            }
            break;
        case QS_MP_RECORDS:
            if (isRemove) {
                filt_.glb[3] &= static_cast<std::uint8_t>(~0x03U & 0xFFU);
                filt_.glb[5] &= static_cast<std::uint8_t>(~0x80U & 0xFFU);
            }
            else {
                filt_.glb[3] |= 0x03U;
                filt_.glb[5] |= 0x80U;
            }
            break;
        case QS_QF_RECORDS:
            if (isRemove) {
                filt_.glb[2] &= static_cast<std::uint8_t>(~0x80U & 0xFFU);
                filt_.glb[3] &= static_cast<std::uint8_t>(~0xFCU & 0xFFU);
                filt_.glb[4] &= static_cast<std::uint8_t>(~0xC0U & 0xFFU);
                filt_.glb[5] &= static_cast<std::uint8_t>(~0x1FU & 0xFFU);
            }
            else {
                filt_.glb[2] |= 0x80U;
                filt_.glb[3] |= 0xFCU;
                filt_.glb[4] |= 0xC0U;
                filt_.glb[5] |= 0x1FU;
            }
            break;
        case QS_TE_RECORDS:
            if (isRemove) {
                filt_.glb[4] &= static_cast<std::uint8_t>(~0x3FU & 0xFFU);
            }
            else {
                filt_.glb[4] |= 0x3FU;
            }
            break;
        case QS_SC_RECORDS:
            if (isRemove) {
                filt_.glb[6] &= static_cast<std::uint8_t>(~0x3FU & 0xFFU);
            }
            else {
               filt_.glb[6] |= 0x3FU;
            }
            break;
        case QS_SEM_RECORDS:
            if (isRemove) {
                filt_.glb[8] &= static_cast<std::uint8_t>(~0x80U & 0xFFU);
                filt_.glb[9] &= static_cast<std::uint8_t>(~0x07U & 0xFFU);
            }
            else {
                filt_.glb[8] |= 0x80U;
                filt_.glb[9] |= 0x07U;
            }
            break;
        case QS_MTX_RECORDS:
            if (isRemove) {
                filt_.glb[9]  &= static_cast<std::uint8_t>(~0xF8U & 0xFFU);
                filt_.glb[10] &= static_cast<std::uint8_t>(~0x01U & 0xFFU);
            }
            else {
                filt_.glb[9]  |= 0xF8U;
                filt_.glb[10] |= 0x01U;
            }
            break;
        case QS_U0_RECORDS:
            if (isRemove) {
                filt_.glb[12] &= static_cast<std::uint8_t>(~0xF0U & 0xFFU);
                filt_.glb[13] &= static_cast<std::uint8_t>(~0x01U & 0xFFU);
            }
            else {
                filt_.glb[12] |= 0xF0U;
                filt_.glb[13] |= 0x01U;
            }
            break;
        case QS_U1_RECORDS:
            if (isRemove) {
                filt_.glb[13] &= static_cast<std::uint8_t>(~0x3EU & 0xFFU);
            }
            else {
                filt_.glb[13] |= 0x3EU;
            }
            break;
        case QS_U2_RECORDS:
            if (isRemove) {
                filt_.glb[13] &= static_cast<std::uint8_t>(~0xC0U & 0xFFU);
                filt_.glb[14] &= static_cast<std::uint8_t>(~0x07U & 0xFFU);
            }
            else {
                filt_.glb[13] |= 0xC0U;
                filt_.glb[14] |= 0x07U;
            }
            break;
        case QS_U3_RECORDS:
            if (isRemove) {
                filt_.glb[14] &= static_cast<std::uint8_t>(~0xF8U & 0xFFU);
            }
            else {
                filt_.glb[14] |= 0xF8U;
            }
            break;
        case QS_U4_RECORDS:
            if (isRemove) {
                filt_.glb[15] &= static_cast<std::uint8_t>(~0x1FU & 0xFFU);
            }
            else {
                filt_.glb[15] |= 0x1FU;
            }
            break;
        case QS_UA_RECORDS:
            if (isRemove) {
                filt_.glb[12] &= static_cast<std::uint8_t>(~0xF0U & 0xFFU);
                filt_.glb[13] = 0U;
                filt_.glb[14] = 0U;
                filt_.glb[15] &= static_cast<std::uint8_t>(~0x1FU & 0xFFU);
            }
            else {
                filt_.glb[12] |= 0xF0U;
                filt_.glb[13] |= 0xFFU;
                filt_.glb[14] |= 0xFFU;
                filt_.glb[15] |= 0x1FU;
            }
            break;
        default: {
            QS_CRIT_STAT
            QS_CRIT_ENTRY();
            // QS rec number must be below 0x7D, so no need for escaping
            Q_ASSERT_INCRIT(210, rec < 0x7DU);
            QS_CRIT_EXIT();

            if (isRemove) {
                filt_.glb[rec >> 3U]
                    &= static_cast<std::uint8_t>(~(1U << (rec & 7U)) & 0xFFU);
            }
            else {
                filt_.glb[rec >> 3U]
                    |= static_cast<std::uint8_t>(1U << (rec & 7U));
                // never turn the last 3 records on (0x7D, 0x7E, 0x7F)
                filt_.glb[15] &= 0x1FU;
            }
            break;
        }
    }
}

//............................................................................
void locFilter_(std::int_fast16_t const filter) noexcept {
    bool const isRemove = (filter < 0);
    std::uint16_t const qsId = isRemove
                  ? static_cast<std::uint16_t>(-filter)
                  : static_cast<std::uint16_t>(filter);
    std::uint8_t const tmp = (isRemove ? 0x00U : 0xFFU);
    std::uint_fast8_t i;
    switch (qsId) {
        case QS_ALL_IDS:
            // set all local filters (partially unrolled loop)
            for (i = 0U; i < Q_DIM(filt_.loc); i += 4U) {
                filt_.loc[i     ] = tmp;
                filt_.loc[i + 1U] = tmp;
                filt_.loc[i + 2U] = tmp;
                filt_.loc[i + 3U] = tmp;
            }
            break;
        case QS_AO_IDS:
            for (i = 0U; i < 8U; i += 4U) {
                filt_.loc[i     ] = tmp;
                filt_.loc[i + 1U] = tmp;
                filt_.loc[i + 2U] = tmp;
                filt_.loc[i + 3U] = tmp;
            }
            break;
        case QS_EP_IDS:
            i = 8U;
            filt_.loc[i     ] = tmp;
            filt_.loc[i + 1U] = tmp;
            break;
        case QS_AP_IDS:
            i = 12U;
            filt_.loc[i     ] = tmp;
            filt_.loc[i + 1U] = tmp;
            filt_.loc[i + 2U] = tmp;
            filt_.loc[i + 3U] = tmp;
            break;
        default: {
            QS_CRIT_STAT
            QS_CRIT_ENTRY();
            // qsId must be in range
            Q_ASSERT_INCRIT(310, qsId < 0x7FU);
            QS_CRIT_EXIT();
            if (isRemove) {
                filt_.loc[qsId >> 3U] &=
                    static_cast<std::uint8_t>(
                        ~(1U << (qsId & 7U)) & 0xFFU);
            }
            else {
                filt_.loc[qsId >> 3U]
                    |= (1U << (qsId & 7U));
            }
            break;
        }
    }
    filt_.loc[0] |= 0x01U; // leave QS_ID == 0 always on
}

//............................................................................
void beginRec_(std::uint_fast8_t const rec) noexcept {
    std::uint8_t const b = priv_.seq + 1U;
    std::uint8_t chksum = 0U;             // reset the checksum
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.seq = b; // store the incremented sequence num
    priv_.used = (priv_.used + 2U); // 2 bytes about to be added

    QS_INSERT_ESC_BYTE_(b)

    chksum += static_cast<std::uint8_t>(rec);
    QS_INSERT_BYTE_(static_cast<std::uint8_t>(rec)) // no need for escaping

    priv_.head   = head;   // save the head
    priv_.chksum = chksum; // save the checksum
}

//............................................................................
void endRec_() noexcept {
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;
    QSCtr const end = priv_.end;
    std::uint8_t b  = priv_.chksum;
    b ^= 0xFFU; // invert the bits in the checksum

    priv_.used = (priv_.used + 2U); // 2 bytes about to be added

    if ((b != QS_FRAME) && (b != QS_ESC)) {
        QS_INSERT_BYTE_(b)
    }
    else {
        QS_INSERT_BYTE_(QS_ESC)
        QS_INSERT_BYTE_(b ^ QS_ESC_XOR)
        priv_.used = (priv_.used + 1U); // account for the ESC byte
    }

    QS_INSERT_BYTE_(QS_FRAME) // do not escape this QS_FRAME

    priv_.head = head; // save the head
    if (priv_.used > end) { // overrun over the old data?
        priv_.used = end;   // the whole buffer is used
        priv_.tail = head;  // shift the tail to the old data
    }
}

//............................................................................
void u8_raw_(std::uint8_t const d) noexcept {
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 1U);  // 1 byte about to be added
    QS_INSERT_ESC_BYTE_(d)

    priv_.head   = head;   // save the head
    priv_.chksum = chksum; // save the checksum
}

//............................................................................
void u8u8_raw_(
    std::uint8_t const d1,
    std::uint8_t const d2) noexcept
{
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 2U); // 2 bytes about to be added
    QS_INSERT_ESC_BYTE_(d1)
    QS_INSERT_ESC_BYTE_(d2)

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
}

//............................................................................
void u16_raw_(std::uint16_t d) noexcept {
    std::uint8_t b = static_cast<std::uint8_t>(d);
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 2U); // 2 bytes about to be added

    QS_INSERT_ESC_BYTE_(b)

    d >>= 8U;
    b = static_cast<std::uint8_t>(d);
    QS_INSERT_ESC_BYTE_(b)

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
}

//............................................................................
void u32_raw_(std::uint32_t d) noexcept {
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 4U); // 4 bytes about to be added
    for (std::uint_fast8_t i = 4U; i != 0U; --i) {
        std::uint8_t const b = static_cast<std::uint8_t>(d);
        QS_INSERT_ESC_BYTE_(b)
        d >>= 8U;
    }

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
}

//............................................................................
void obj_raw_(void const * const obj) noexcept {
    #if (QS_OBJ_PTR_SIZE == 1U)
        u8_raw_(reinterpret_cast<std::uint8_t>(obj));
    #elif (QS_OBJ_PTR_SIZE == 2U)
        u16_raw_(reinterpret_cast<std::uint16_t>(obj));
    #elif (QS_OBJ_PTR_SIZE == 4U)
        u32_raw_(reinterpret_cast<std::uint32_t>(obj));
    #elif (QS_OBJ_PTR_SIZE == 8U)
        u64_raw_(reinterpret_cast<std::uint64_t>(obj));
    #else
        u32_raw_(reinterpret_cast<std::uint32_t>(obj));
    #endif
}

//............................................................................
void str_raw_(char const * s) noexcept {
    std::uint8_t b = static_cast<std::uint8_t>(*s);
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)
    QSCtr used      = priv_.used;         // put in a temporary (register)

    while (b != 0U) {
        chksum += b;       // update checksum
        QS_INSERT_BYTE_(b) // ASCII characters don't need escaping
        ++s;
        b = static_cast<std::uint8_t>(*s);
        ++used;
    }
    QS_INSERT_BYTE_(0U) // zero-terminate the string
    ++used;

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
    priv_.used   = used;    // save # of used buffer space
}

//............................................................................
void u8_fmt_(
    std::uint8_t const format,
    std::uint8_t const d) noexcept
{
    std::uint8_t chksum = priv_.chksum;  // put in a temporary (register)
    std::uint8_t *const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;        // put in a temporary (register)
    QSCtr const end = priv_.end;         // put in a temporary (register)

    priv_.used = (priv_.used + 2U); // 2 bytes about to be added

    QS_INSERT_ESC_BYTE_(format)
    QS_INSERT_ESC_BYTE_(d)

    priv_.head   = head;   // save the head
    priv_.chksum = chksum; // save the checksum
}

//............................................................................
void u16_fmt_(
    std::uint8_t format,
    std::uint16_t d) noexcept
{
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 3U); // 3 bytes about to be added

    QS_INSERT_ESC_BYTE_(format)

    format = static_cast<std::uint8_t>(d);
    QS_INSERT_ESC_BYTE_(format)

    d >>= 8U;
    format = static_cast<std::uint8_t>(d);
    QS_INSERT_ESC_BYTE_(format)

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
}

//............................................................................
void u32_fmt_(
    std::uint8_t format,
    std::uint32_t d) noexcept
{
    std::uint8_t chksum = priv_.chksum;   // put in a temporary (register)
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + 5U); // 5 bytes about to be added
    QS_INSERT_ESC_BYTE_(format) // insert the format byte

    for (std::uint_fast8_t i = 4U; i != 0U; --i) {
        format = static_cast<std::uint8_t>(d);
        QS_INSERT_ESC_BYTE_(format)
        d >>= 8U;
    }

    priv_.head   = head;   // save the head
    priv_.chksum = chksum; // save the checksum
}

//............................................................................
void str_fmt_(char const * s) noexcept {
    std::uint8_t b      = static_cast<std::uint8_t>(*s);
    std::uint8_t chksum = static_cast<std::uint8_t>(
                          priv_.chksum + static_cast<std::uint8_t>(STR_T));
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)
    QSCtr used      = priv_.used;         // put in a temporary (register)

    used += 2U; // the format byte and the terminating-0

    QS_INSERT_BYTE_(static_cast<std::uint8_t>(STR_T))
    while (b != 0U) {
        // ASCII characters don't need escaping
        chksum += b;  // update checksum
        QS_INSERT_BYTE_(b)
        ++s;
        b = static_cast<std::uint8_t>(*s);
        ++used;
    }
    QS_INSERT_BYTE_(0U) // zero-terminate the string

    priv_.head   = head;   // save the head
    priv_.chksum = chksum; // save the checksum
    priv_.used   = used;   // save # of used buffer space
}

//............................................................................
void mem_fmt_(
    std::uint8_t const * blk,
    std::uint8_t size) noexcept
{
    std::uint8_t b = static_cast<std::uint8_t>(MEM_T);
    std::uint8_t chksum = priv_.chksum + b;
    std::uint8_t * const buf = priv_.buf; // put in a temporary (register)
    QSCtr head      = priv_.head;         // put in a temporary (register)
    QSCtr const end = priv_.end;          // put in a temporary (register)

    priv_.used = (priv_.used + size + 2U); // size+2 bytes to be added

    QS_INSERT_BYTE_(b)
    QS_INSERT_ESC_BYTE_(size)

    // output the 'size' number of bytes
    for (; size != 0U; --size) {
        b = *blk;
        QS_INSERT_ESC_BYTE_(b)
        ++blk;
    }

    priv_.head   = head;    // save the head
    priv_.chksum = chksum;  // save the checksum
}

//............................................................................
void sig_dict_pre_(
    QSignal const sig,
    void const * const obj,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_SIG_DICT));
    QS_SIG_PRE_(sig);
    QS_OBJ_PRE_(obj);
    QS_STR_PRE_((*name == '&') ? &name[1] : name);
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void obj_dict_pre_(
    void const * const obj,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_OBJ_DICT));
    QS_OBJ_PRE_(obj);
    QS_STR_PRE_((*name == '&') ? &name[1] : name);
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void obj_arr_dict_pre_(
    void const * const obj,
    std::uint_fast16_t const idx,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    Q_REQUIRE_INCRIT(400, idx < 1000U);
    QS_CRIT_EXIT();

    // format idx into a char buffer as "xxx\0"
    std::uint8_t idx_str[4];
    std::uint_fast16_t tmp = idx;
    std::uint8_t i;
    idx_str[3] = 0U; // zero-terminate
    idx_str[2] = static_cast<std::uint8_t>(
                    static_cast<std::uint8_t>('0') + (tmp % 10U));
    tmp /= 10U;
    idx_str[1] = static_cast<std::uint8_t>(
                    static_cast<std::uint8_t>('0') + (tmp % 10U));
    if (idx_str[1] == static_cast<std::uint8_t>('0')) {
       i = 2U;
    }
    else {
       tmp /= 10U;
       idx_str[0] = static_cast<std::uint8_t>(
                        static_cast<std::uint8_t>('0') + (tmp % 10U));
        if (idx_str[0] == static_cast<std::uint8_t>('0')) {
           i = 1U;
        }
        else {
           i = 0U;
        }
    }

    std::uint8_t j = ((*name == '&') ? 1U : 0U);

    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_OBJ_DICT));
    QS_OBJ_PRE_(obj);
    for (; name[j] != '\0'; ++j) {
        QS_U8_PRE_(name[j]);
        if (name[j] == '[') {
            ++j;
            break;
        }
    }
    for (; idx_str[i] != 0U; ++i) {
        QS_U8_PRE_(idx_str[i]);
    }
    // skip chars until ']'
    for (; name[j] != '\0'; ++j) {
        if (name[j] == ']') {
            break;
        }
    }
    for (; name[j] != '\0'; ++j) {
        QS_U8_PRE_(name[j]);
    }
    QS_U8_PRE_(0U); // zero-terminate
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void fun_dict_pre_(
    QSpyFunPtr fun,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_FUN_DICT));
    QS_FUN_PRE_(fun);
    QS_STR_PRE_((*name == '&') ? &name[1] : name);
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void usr_dict_pre_(
    enum_t const rec,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_USR_DICT));
    QS_U8_PRE_(rec);
    QS_STR_PRE_(name);
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void enum_dict_pre_(
    enum_t const value,
    std::uint8_t const group,
    char const * const name) noexcept
{
    QS_CRIT_STAT
    QS_CRIT_ENTRY();
    QS_MEM_SYS();

    beginRec_(static_cast<std::uint_fast8_t>(QS_ENUM_DICT));
    QS_2U8_PRE_(static_cast<std::uint8_t>(value), group);
    QS_STR_PRE_(name);
    endRec_();

    QS_MEM_APP();
    QS_CRIT_EXIT();
    onFlush();
}

//............................................................................
void assertion_pre_(
    char const * const module,
    int_t const id,
    std::uint32_t const delay) noexcept
{
    // NOTE: called in a critical section

    beginRec_(static_cast<std::uint_fast8_t>(QS_ASSERT_FAIL));
        QS_TIME_PRE_();
        QS_U16_PRE_(id);
        QS_STR_PRE_((module != nullptr) ? module : "?");
    endRec_();
    onFlush();

    // busy-wait until all QS data makes it over to the host
    for (std::uint32_t volatile ctr = delay; ctr > 0U; ) {
        ctr = (ctr - 1U);
    }
    QS::onCleanup();
}

//............................................................................
void crit_entry_pre_() noexcept {
    beginRec_(static_cast<std::uint_fast8_t>(QS_QF_CRIT_ENTRY));
        QS_TIME_PRE_();
        priv_.critNest = (priv_.critNest + 1U);
        QS_U8_PRE_(priv_.critNest);
    endRec_();
}

//............................................................................
void crit_exit_pre_() noexcept {
    beginRec_(static_cast<std::uint_fast8_t>(QS_QF_CRIT_EXIT));
        QS_TIME_PRE_();
        QS_U8_PRE_(QS::priv_.critNest);
        priv_.critNest = (priv_.critNest - 1U);
    endRec_();
}

//............................................................................
void isr_entry_pre_(
    std::uint8_t const isrnest,
    std::uint8_t const prio) noexcept
{
    beginRec_(static_cast<std::uint_fast8_t>(QS_QF_ISR_ENTRY));
        QS_TIME_PRE_();
        QS_U8_PRE_(isrnest);
        QS_U8_PRE_(prio);
    endRec_();
}

//............................................................................
void isr_exit_pre_(
    std::uint8_t const isrnest,
    std::uint8_t const prio) noexcept
{
    beginRec_(static_cast<std::uint_fast8_t>(QS_QF_ISR_EXIT));
        QS_TIME_PRE_();
        QS_U8_PRE_(isrnest);
        QS_U8_PRE_(prio);
    endRec_();
}

//............................................................................
void target_info_pre_(std::uint8_t const isReset) {
    // NOTE: called in a critical section

    static constexpr std::uint8_t ZERO = static_cast<std::uint8_t>('0');
    static std::uint8_t const * const TIME =
        reinterpret_cast<std::uint8_t const *>(&BUILD_TIME[0]);
    static std::uint8_t const * const DATE =
        reinterpret_cast<std::uint8_t const *>(&BUILD_DATE[0]);

    beginRec_(static_cast<std::uint_fast8_t>(QS_TARGET_INFO));
        u8_raw_(isReset);

    static union {
        std::uint16_t u16;
        std::uint8_t  u8[2];
    } endian_test;
    endian_test.u16 = 0x0102U;
    // big endian ? add the 0x8000U flag
    QS_U16_PRE_(((endian_test.u8[0] == 0x01U)
                ? (0x8000U | QP_VERSION)
                : QP_VERSION)); // target endianness + version number

    // send the object sizes...
    u8_raw_(Q_SIGNAL_SIZE
                | static_cast<std::uint8_t>(QF_EVENT_SIZ_SIZE << 4U));

#ifdef QF_EQUEUE_CTR_SIZE
    u8_raw_(QF_EQUEUE_CTR_SIZE
                | static_cast<std::uint8_t>(QF_TIMEEVT_CTR_SIZE << 4U));
#else
    QS::u8_raw_(static_cast<std::uint8_t>(QF_TIMEEVT_CTR_SIZE << 4U));
#endif // ifdef QF_EQUEUE_CTR_SIZE

#ifdef QF_MPOOL_CTR_SIZE
    QS::u8_raw_(QF_MPOOL_SIZ_SIZE
                | static_cast<std::uint8_t>(QF_MPOOL_CTR_SIZE << 4U));
#else
    QS::u8_raw_(0U);
#endif // ifdef QF_MPOOL_CTR_SIZE

    QS::u8_raw_(QS_OBJ_PTR_SIZE | (QS_FUN_PTR_SIZE << 4U));
    QS::u8_raw_(QS_TIME_SIZE);

    // send the limits...
    QS::u8_raw_(QF_MAX_ACTIVE);
    QS::u8_raw_(QF_MAX_EPOOL | (QF_MAX_TICK_RATE << 4U));

    // send the build time in three bytes (sec, min, hour)...
    QS::u8_raw_((10U * (TIME[6] - ZERO)) + (TIME[7] - ZERO));
    QS::u8_raw_((10U * (TIME[3] - ZERO)) + (TIME[4] - ZERO));
    if (BUILD_TIME[0] == static_cast<std::uint8_t>(' ')) {
        QS::u8_raw_(TIME[1] - ZERO);
    }
    else {
        QS::u8_raw_((10U * (TIME[0] - ZERO)) + (TIME[1] - ZERO));
    }

    // send the build date in three bytes (day, month, year) ...
    if (BUILD_DATE[4] == static_cast<std::uint8_t>(' ')) {
        QS::u8_raw_(DATE[5] - ZERO);
    }
    else {
        QS::u8_raw_((10U * (DATE[4] - ZERO)) + (DATE[5] - ZERO));
    }
    // convert the 3-letter month to a number 1-12 ...
    std::uint8_t b;
    switch (DATE[0] + DATE[1] + DATE[2]) {
        case 'J' + 'a' +'n':
            b = 1U;
            break;
        case 'F' + 'e' + 'b':
            b = 2U;
            break;
        case 'M' + 'a' +'r':
            b = 3U;
            break;
        case 'A' + 'p' + 'r':
            b = 4U;
            break;
        case 'M' + 'a' + 'y':
            b = 5U;
            break;
        case 'J' + 'u' + 'n':
            b = 6U;
            break;
        case 'J' + 'u' + 'l':
            b = 7U;
            break;
        case 'A' + 'u' + 'g':
            b = 8U;
            break;
        case 'S' + 'e' + 'p':
            b = 9U;
            break;
        case 'O' + 'c' + 't':
            b = 10U;
            break;
        case 'N' + 'o' + 'v':
            b = 11U;
            break;
        case 'D' + 'e' + 'c':
            b = 12U;
            break;
        default:
            b = 0U;
            break;
    }
    QS::u8_raw_(b); // store the month
    QS::u8_raw_((10U * (DATE[9] - ZERO)) + (DATE[10] - ZERO));
    QS::endRec_();
}

} // namespace QS
} // namespace QP

//! @endcond
