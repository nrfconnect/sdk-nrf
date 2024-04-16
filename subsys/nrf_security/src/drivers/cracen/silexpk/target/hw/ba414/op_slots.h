/**
 * \brief BA414ep hardware operand slot indexes for each command
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OP_SLOTS_HEADERFILE
#define OP_SLOTS_HEADERFILE

#define OP_SLOT_DEFAULT_PTRS 6
#define OP_SLOT_PTR_A	     6
#define OP_SLOT_PTR_B	     8
#define OP_SLOT_PTR_C	     10
#define OP_SLOT_PTR_AA	     12
#define OP_SLOT_BLIND_FACTOR 15

#define OP_SLOT_ECC_PARAM_P  0
#define OP_SLOT_ECC_PARAM_N  1
#define OP_SLOT_ECC_PARAM_A  4
#define OP_SLOT_ECC_PARAM_B  5
#define OP_SLOT_ECC_PARAM_GX 2
#define OP_SLOT_ECC_PARAM_GY 3

#define OP_SLOT_MOD_EXP_M      0
#define OP_SLOT_MOD_EXP_EXP    OP_SLOT_PTR_B
#define OP_SLOT_MOD_EXP_INPUT  OP_SLOT_PTR_A
#define OP_SLOT_MOD_EXP_RESULT OP_SLOT_PTR_C

#define OP_SLOT_MOD_EXP_CM_M	    0
#define OP_SLOT_MOD_EXP_CM_LAMBDA_N 1
#define OP_SLOT_MOD_EXP_CM_INPUT    4
#define OP_SLOT_MOD_EXP_CM_EXP	    6
#define OP_SLOT_MOD_EXP_CM_RESULT   5
#define OP_SLOT_MOD_EXP_CM_RAND	    15

#define OP_SLOT_RSA_CRT_INPUT  4
#define OP_SLOT_RSA_CRT_P      2
#define OP_SLOT_RSA_CRT_Q      3
#define OP_SLOT_RSA_CRT_DP     10
#define OP_SLOT_RSA_CRT_DQ     11
#define OP_SLOT_RSA_CRT_QINV   12
#define OP_SLOT_RSA_CRT_RESULT 5

#define OP_SLOT_RSA_KEYGEN_P 2
#define OP_SLOT_RSA_KEYGEN_Q 3
#define OP_SLOT_RSA_KEYGEN_E 8
#define OP_SLOT_RSA_KEYGEN_N 0
#define OP_SLOT_RSA_KEYGEN_L 1
#define OP_SLOT_RSA_KEYGEN_D 6

#define OP_SLOT_CRT_KEYPARAMS_P	   2
#define OP_SLOT_CRT_KEYPARAMS_Q	   3
#define OP_SLOT_CRT_KEYPARAMS_D	   6
#define OP_SLOT_CRT_KEYPARAMS_DP   10
#define OP_SLOT_CRT_KEYPARAMS_DQ   11
#define OP_SLOT_CRT_KEYPARAMS_QINV 12

#define OP_SLOT_SRP_KEYPARAMS_N 0
#define OP_SLOT_SRP_KEYPARAMS_G 3
#define OP_SLOT_SRP_KEYPARAMS_A 4
#define OP_SLOT_SRP_KEYPARAMS_B 5
#define OP_SLOT_SRP_KEYPARAMS_X 6
#define OP_SLOT_SRP_KEYPARAMS_K 7
#define OP_SLOT_SRP_KEYPARAMS_U 8
#define OP_SLOT_SRP_KEYPARAMS_S 11

#define OP_SLOT_ECKCDSA_D  6
#define OP_SLOT_ECKCDSA_K  7
#define OP_SLOT_ECKCDSA_QX 8
#define OP_SLOT_ECKCDSA_QY 9
#define OP_SLOT_ECKCDSA_R  10
#define OP_SLOT_ECKCDSA_S  11
#define OP_SLOT_ECKCDSA_H  12
#define OP_SLOT_ECKCDSA_WX 13
#define OP_SLOT_ECKCDSA_WY 14

#define OP_SLOT_SM2_Q 8
#define OP_SLOT_SM2_R 10
#define OP_SLOT_SM2_S 11
#define OP_SLOT_SM2_H 12

#define OP_SLOT_ECDSA_VER_QX 8
#define OP_SLOT_ECDSA_VER_QY 9
#define OP_SLOT_ECDSA_VER_R  10
#define OP_SLOT_ECDSA_VER_S  11
#define OP_SLOT_ECDSA_VER_H  12

#define OP_SLOT_ECDSA_SGN_D 6
#define OP_SLOT_ECDSA_SGN_K 7
#define OP_SLOT_ECDSA_SGN_H 12
#define OP_SLOT_ECDSA_SGN_R 10
#define OP_SLOT_ECDSA_SGN_S 11

#define OP_SLOT_SM2_D	6
#define OP_SLOT_SM2_K	7
#define OP_SLOT_SM2_Q	8
#define OP_SLOT_SM2_RB	10
#define OP_SLOT_SM2_COF 12
#define OP_SLOT_SM2_RA	13
#define OP_SLOT_SM2_U	13
#define OP_SLOT_SM2_W	15

#define OP_SLOT_ECC_PTMUL_K OP_SLOT_PTR_B
#define OP_SLOT_ECC_PTMUL_P OP_SLOT_PTR_AA
#define OP_SLOT_ECC_PTMUL_R OP_SLOT_PTR_C

#define OP_SLOT_ECC_PT_DOUBLE_P OP_SLOT_PTR_A
#define OP_SLOT_ECC_PT_DOUBLE_R OP_SLOT_PTR_C

#define OP_SLOT_ECC_PT_ADD_P1 OP_SLOT_PTR_A
#define OP_SLOT_ECC_PT_ADD_P2 OP_SLOT_PTR_B
#define OP_SLOT_ECC_PT_ADD_R  OP_SLOT_PTR_C

#define OP_SLOT_SM2_D 6
#define OP_SLOT_SM2_K 7
#define OP_SLOT_SM2_R 10
#define OP_SLOT_SM2_S 11
#define OP_SLOT_SM2_H 12

#define OP_SLOT_ECC_PTONCURVE_P OP_SLOT_PTR_AA

#define OP_SLOT_EDDSA_P		 0
#define OP_SLOT_EDDSA_L		 1
#define OP_SLOT_EDDSA_BX	 2
#define OP_SLOT_EDDSA_BY	 3
#define OP_SLOT_EDDSA_D		 4
#define OP_SLOT_EDDSA_I		 5
#define OP_SLOT_EDDSA_PTMUL_P	 6
#define OP_SLOT_EDDSA_PTMUL_R	 8
#define OP_SLOT_EDDSA_PTMUL_RX	 10
#define OP_SLOT_EDDSA_PTMUL_RY	 11
#define OP_SLOT_EDDSA_SIGN_K	 6
#define OP_SLOT_EDDSA_SIGN_R	 8
#define OP_SLOT_EDDSA_SIGN_S	 11
#define OP_SLOT_EDDSA_SIGN_SIG_S 10
#define OP_SLOT_EDDSA_VER_K	 6
#define OP_SLOT_EDDSA_VER_R	 8
#define OP_SLOT_EDDSA_VER_AY	 9
#define OP_SLOT_EDDSA_VER_RY	 11
#define OP_SLOT_EDDSA_VER_S	 10

#define OP_SLOT_MILLER_RABIN_N 0
#define OP_SLOT_MILLER_RABIN_A 6

#define OP_SLOT_DSA_P 0
#define OP_SLOT_DSA_Q 2
#define OP_SLOT_DSA_G 3
#define OP_SLOT_DSA_K 5
#define OP_SLOT_DSA_X 6
#define OP_SLOT_DSA_Y 8
#define OP_SLOT_DSA_R 10
#define OP_SLOT_DSA_S 11
#define OP_SLOT_DSA_H 12

#define OP_SLOT_IK_PUBKEY_Q 8
#define OP_SLOT_IK_PUBKEY_R 8
#define OP_SLOT_IK_PUBKEY_P 10

/* SM9: Common */
#define OP_SLOT_SM9_H (0x0c)
#define OP_SLOT_SM9_T (0x0e)

/* SM9: Exponentiation in GT */
#define OP_SLOT_SM9_G (0x10)
#define OP_SLOT_SM9_Z (0x10)

/* SM9: Multiplication in G1 */
#define OP_SLOT_SM9_P1_X0    (0x02)
#define OP_SLOT_SM9_P1_Y0    (0x03)
#define OP_SLOT_SM9_PPUBE_X0 (0x06)
#define OP_SLOT_SM9_PPUBE_Y0 (0x07)
#define OP_SLOT_SM9_KE	     (0x0d)

/* SM9: Multiplication in G2 */
#define OP_SLOT_SM9_P2_X0    (0x02)
#define OP_SLOT_SM9_P2_X1    (0x03)
#define OP_SLOT_SM9_P2_Y0    (0x04)
#define OP_SLOT_SM9_P2_Y1    (0x05)
#define OP_SLOT_SM9_PPUBS_X0 (0x06)
#define OP_SLOT_SM9_PPUBS_X1 (0x07)
#define OP_SLOT_SM9_PPUBS_Y0 (0x08)
#define OP_SLOT_SM9_PPUBS_Y1 (0x09)

/* SM9: Pairing */
#define OP_SLOT_SM9_PX0 (0x0a)
#define OP_SLOT_SM9_PY0 (0x0b)
#define OP_SLOT_SM9_QX0 (0x02)
#define OP_SLOT_SM9_QX1 (0x03)
#define OP_SLOT_SM9_QY0 (0x04)
#define OP_SLOT_SM9_QY1 (0x05)
#define OP_SLOT_SM9_F	(0x0d)
#define OP_SLOT_SM9_T	(0x0e)

#define OP_SLOT_SM9_R_12 (0x10)

/* SM9: Sign private key */
#define OP_SLOT_SM9_P1_X0 (0x02)
#define OP_SLOT_SM9_P1_Y0 (0x03)
#define OP_SLOT_SM9_KS	  (0x0d)

/* SM9: Signature generation */
#define OP_SLOT_SM9_DS_X0 (0x06)
#define OP_SLOT_SM9_DS_Y0 (0x07)
#define OP_SLOT_SM9_S_X0  (0x0a)
#define OP_SLOT_SM9_S_Y0  (0x0b)

/* SM9: Signature verification */
#define OP_SLOT_SM9_H1	  (0x01)
#define OP_SLOT_SM9_P2_X0 (0x02)
#define OP_SLOT_SM9_P2_X1 (0x03)
#define OP_SLOT_SM9_P2_Y0 (0x04)
#define OP_SLOT_SM9_P2_Y1 (0x05)

#define OP_SLOT_SM9_PPUBS_X0 (0x06)
#define OP_SLOT_SM9_PPUBS_X1 (0x07)
#define OP_SLOT_SM9_PPUBS_Y0 (0x08)
#define OP_SLOT_SM9_PPUBS_Y1 (0x09)

#define OP_SLOT_SM9_SX0 (0x0a)
#define OP_SLOT_SM9_SY0 (0x0b)

#define OP_SLOT_SM9_OUTDE_X0 (0x06)
#define OP_SLOT_SM9_OUTDE_X1 (0x07)
#define OP_SLOT_SM9_OUTDE_Y0 (0x08)
#define OP_SLOT_SM9_OUTDE_Y1 (0x09)

/* SM9: Send key */
#define OP_SLOT_SM9_P1_X0     (0x02)
#define OP_SLOT_SM9_P1_Y0     (0x03)
#define OP_SLOT_SM9_PPUBE_X0  (0x06)
#define OP_SLOT_SM9_PPUBE_Y0  (0x07)
#define OP_SLOT_SM9_PPUBE_RX0 (0x0a)
#define OP_SLOT_SM9_PPUBE_RY0 (0x0b)
#define OP_SLOT_SM9_R	      (0x0d)

/* SM9: Reduce H */
#define OP_SLOT_SM9_H (0x0c)

/* EC J-PAKE: Generate ZKP */
#define OP_SLOT_ECJPAKE_V (0x08)
#define OP_SLOT_ECJPAKE_X (0x0b)
#define OP_SLOT_ECJPAKE_H (0x0c)
#define OP_SLOT_ECJPAKE_R (0x0a)

/* EC J-PAKE: Verify ZKP */
#define OP_SLOT_ECJPAKE_XG_1 (0x02)
#define OP_SLOT_ECJPAKE_YG_1 (0x03)
#define OP_SLOT_ECJPAKE_XV   (0x06)
#define OP_SLOT_ECJPAKE_YV   (0x07)
#define OP_SLOT_ECJPAKE_XX   (0x08)
#define OP_SLOT_ECJPAKE_YX   (0x09)
#define OP_SLOT_ECJPAKE_XG_2 (0x0d)
#define OP_SLOT_ECJPAKE_YG_2 (0x0e)

/* EC J-PAKE: 3 point add */
#define OP_SLOT_ECJPAKE_X2_1 (0x06)
#define OP_SLOT_ECJPAKE_X2_2 (0x07)
#define OP_SLOT_ECJPAKE_X3_1 (0x08)
#define OP_SLOT_ECJPAKE_X3_2 (0x09)
#define OP_SLOT_ECJPAKE_X1_1 (0x0b)
#define OP_SLOT_ECJPAKE_X1_2 (0x0c)
#define OP_SLOT_ECJPAKE_GB_1 (0x0d)
#define OP_SLOT_ECJPAKE_GB_2 (0x0e)

/* EC J-PAKE: Generate step 2 */
#define OP_SLOT_ECJPAKE_X4_1 (0x06)
#define OP_SLOT_ECJPAKE_X4_2 (0x07)
#define OP_SLOT_ECJPAKE_B_1  (0x08)
#define OP_SLOT_ECJPAKE_B_2  (0x09)
#define OP_SLOT_ECJPAKE_X2SS (0x0a)
#define OP_SLOT_ECJPAKE_S    (0x0d)

#define OP_SLOT_ECJPAKE_AX  (0x08)
#define OP_SLOT_ECJPAKE_AY  (0x09)
#define OP_SLOT_ECJPAKE_X2S (0x0b)
#define OP_SLOT_ECJPAKE_GAX (0x0d)
#define OP_SLOT_ECJPAKE_GAY (0x0e)

/* EC J-PAKE: Generate session key */
#define OP_SLOT_ECJPAKE_B_1 (0x08)
#define OP_SLOT_ECJPAKE_B_2 (0x09)
#define OP_SLOT_ECJPAKE_X2  (0x0a)
#define OP_SLOT_ECJPAKE_X2S (0x0b)
#define OP_SLOT_ECJPAKE_T_1 (0x0d)
#define OP_SLOT_ECJPAKE_T_2 (0x0e)

/* SRP: Server public key generation */
#define OP_SLOT_SRP_B	 (0x05)
#define OP_SLOT_SRP_N	 (0x00)
#define OP_SLOT_SRP_G	 (0x03)
#define OP_SLOT_SRP_K	 (0x07)
#define OP_SLOT_SRP_V	 (0x0a)
#define OP_SLOT_SRP_BRND (0x0c)

/* SRP: Server session key generation */
#define OP_SLOT_SRP_S (0x0b)
#define OP_SLOT_SRP_A (0x02)
#define OP_SLOT_SRP_U (0x08)

#endif
