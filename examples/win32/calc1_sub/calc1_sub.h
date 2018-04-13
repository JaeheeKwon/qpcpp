//$file${.::calc1_sub.h} #####################################################
//
// Model: calc1_sub.qm
// File:  ${.::calc1_sub.h}
//
// This code has been generated by QM tool (https://state-machine.com/qm).
// DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
//
// This code is covered by the following QP license:
// License #   : QPCPP-EVAL-181231
// Issued to   : Institution or an individual evaluating the QP frameworks
// Framework(s): qpcpp
// Support ends: 2018-12-31
// Product(s)  :
// This license is available only for evaluation purposes and
// the generated code is still licensed under the terms of GPL.
// Please submit request for extension of the evaluaion period at:
// http://www.state-machine.com/licensing/#RequestForm
//
//$endhead${.::calc1_sub.h} ##################################################
#ifndef calc1_sub_h
#define calc1_sub_h

enum CalcSignals {
    C_SIG = QP::Q_USER_SIG,
    CE_SIG,
    DIGIT_0_SIG,
    DIGIT_1_9_SIG,
    POINT_SIG,
    OPER_SIG,
    EQUALS_SIG,
    OFF_SIG
};

//$declare${Events::CalcEvt} #################################################
//${Events::CalcEvt} .........................................................
class CalcEvt : public QP::QEvt {
public:
    uint8_t key_code;
};
//$enddecl${Events::CalcEvt} #################################################

extern QP::QMsm * const the_calc; // "opaque" pointer to calculator MSM

#endif // calc1_sub_h