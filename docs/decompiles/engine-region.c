// ==== EngineTick @ 004232e0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 EngineTick(float param_1)

{
  float fVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  undefined **ppuVar5;
  uint uVar6;
  char *pcVar7;
  int *piVar8;
  float fVar9;
  float fVar10;
  uint uVar11;
  int iVar12;
  float10 fVar13;
  undefined8 uVar14;
  float fStack_34;
  undefined4 uStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  float fStack_1c;
  undefined4 uStack_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  float fStack_c;
  float fStack_8;
  
  fVar2 = param_1;
  uStack_30 = 0;
  if ((DAT_00505f88 == 10) && (iVar3 = FUN_00435940(), iVar3 == 0)) {
    return 0;
  }
  if ((DAT_00556ecc & 0x100000) != 0) {
    param_1 = 0.0;
  }
  FUN_004231f0();
  _DAT_00556f00 = param_1;
  if ((DAT_00556ec8 == 0) && (DAT_00505f88 < 2)) {
    fStack_2c = param_1;
  }
  else {
    fVar13 = (float10)FUN_0041fb40();
    fStack_2c = (float)(fVar13 * (float10)param_1);
  }
  fVar10 = fStack_2c;
  FUN_0046a730();
  FUN_00435a50();
  uVar4 = FUN_00431560();
  FUN_0042c440(uVar4);
  FUN_0042cb70();
  FUN_004261c0();
  if ((DAT_00556ed0 & 1) == 0) {
    if (((DAT_005560ec != 0) && (*(int *)(DAT_005560ec + 0x24) != 0)) && (DAT_0055610c != 0)) {
      FUN_00473b40(DAT_0055610c,1);
    }
  }
  else if (((DAT_005560ec != 0) && (*(int *)(DAT_005560ec + 0x24) != 0)) &&
          ((DAT_0055610c != 0 && ((DAT_00556ecc & 0x100000) == 0)))) {
    FUN_00473b40(DAT_0055610c,0);
    FUN_00475580(DAT_0055610c,2,0,0x3f800000,0,param_1 * _DAT_0049e2c0);
  }
  if (((DAT_00556ecc & 0x100000) == 0) && (((DAT_00556ecc & 0x200000) != 0 || (DAT_00556158 != 0))))
  {
    if (DAT_00556158 != 0) {
      fStack_28 = (_DAT_0049e2b0 / param_1) * _DAT_0049e2c4;
      FUN_004754c0(*(undefined4 *)(DAT_005560e4 + 0x2c),0,&fStack_c);
      DAT_00556148 = (fStack_28 * fStack_c + _DAT_00556150) / (fStack_28 - _DAT_0049e2c8);
      DAT_0055614c = (fStack_28 * fStack_8 + _DAT_00556154) / (fStack_28 - _DAT_0049e2c8);
      fStack_28 = DAT_00556148 - fStack_c;
      fStack_24 = DAT_0055614c - fStack_8;
      if (SQRT(fStack_24 * fStack_24 + fStack_28 * fStack_28) < _DAT_0049e2cc) {
        DAT_00556158 = 0;
      }
    }
    FUN_00436b0b(DAT_005560e4,DAT_00556148,DAT_0055614c);
    DAT_00556ecc = DAT_00556ecc & 0xffdfffff;
  }
  if ((DAT_00556ecc & 4) != 0) {
    fStack_2c = 1.0;
    DAT_00556ecc = DAT_00556ecc & 0xfffffffb;
    fVar10 = 1.0;
  }
  iVar3 = FUN_004250c0(fVar10,param_1,&uStack_10,&uStack_14,&uStack_18);
  if (iVar3 == 0) {
    return 0;
  }
  if ((DAT_00556ecc & 0x100000) == 0) {
    FUN_00425d90();
  }
  FUN_00402220();
  FUN_00424600(DAT_005560d4,fVar10);
  FUN_0040aa10();
  FUN_0043b270();
  FUN_0046f550();
  FUN_0040eab0();
  FUN_004313b0(DAT_005560d4,fVar10);
  FUN_004520d0();
  FUN_00435f5c(DAT_005560e4,DAT_005560d4,param_1,fVar10);
  FUN_00435f5c(DAT_005560e8,DAT_005560d4,param_1,fVar10);
  FUN_00435f5c(DAT_005560ec,DAT_005560d4,param_1,fVar10);
  FUN_004091e0();
  FUN_0041ba10();
  FUN_004291a0(param_1,0);
  FUN_00449c00();
  FUN_0040d450();
  FUN_00426520(fVar2,param_1,uStack_10,uStack_14,uStack_18);
  FUN_0044eff0(*(undefined4 *)(DAT_005560d4 + 4),fVar10);
  FUN_0046e390(DAT_005560d4,fVar10);
  FUN_0047aba0();
  FUN_00465b00();
  FUN_004665c0();
  FUN_0044cfa0();
  FUN_0041a2c0();
  if (DAT_00556ec0 != 0) {
    FUN_0040f0e0();
  }
  if ((((DAT_00556ed0 & 0x10) != 0) && (DAT_0076abc0 != '\0')) && (DAT_0076aa40 != DAT_0076abc0)) {
    DAT_00556ec0 = (uint)(DAT_00556ec0 == 0);
    ppuVar5 = (undefined **)&DAT_004a40bc;
    if (DAT_00556ec0 == 0) {
      ppuVar5 = &PTR_DAT_004a40b8;
    }
    FUN_00485230(DAT_00556140,s_Fallin_Mode__s_004a40a8,ppuVar5);
  }
  FUN_00473b40(DAT_00556100,0);
  if (DAT_00556ec8 == 1) {
    if ((DAT_00556ecc & 0x100000) == 0) {
      FUN_00473b40(DAT_00556104,0);
    }
    FUN_00473b40(DAT_005560f0,0);
    FUN_00477010(DAT_005560dc,1);
    FUN_00477150(DAT_005560dc,DAT_005560d8,param_1);
    FUN_00473b40(DAT_005560f0,1);
    FUN_00473b40(DAT_00556104,1);
  }
  else if (DAT_00556ec8 == 0) {
    FUN_004634b0();
    FUN_00463230();
    FUN_0043b850(DAT_00556160,*(undefined4 *)(DAT_005560ec + 0x24),DAT_00556400,DAT_00556404,1);
    uVar4 = FUN_00435f0b(DAT_005560ec,1);
    FUN_0043b820(DAT_00556160,uVar4);
    FUN_00424730(1,fVar10);
    if (((DAT_00556ecc & 0x100000) == 0) && (FUN_00473b40(DAT_005560f8,0), DAT_005560fc != 0)) {
      FUN_00475580(DAT_005560fc,2,0,0x3f800000,0,param_1 * _DAT_0049e2d0);
      FUN_00473b40(DAT_005560fc,0);
    }
    FUN_00477010(DAT_005560dc,1);
    FUN_00477150(DAT_005560dc,DAT_005560d8,param_1);
    FUN_00473b40(DAT_005560f8,1);
    if (DAT_005560fc != 0) {
      FUN_00473b40(DAT_005560fc,1);
    }
    FUN_00424730(0,fVar10);
    uVar4 = FUN_00435f0b(DAT_005560ec,0);
    FUN_0043b820(DAT_00556160,uVar4);
    FUN_0043b850(DAT_00556160,0,0,0,0);
    FUN_00463540();
    FUN_004634b0();
    FUN_00434530();
  }
  FUN_00473b40(DAT_00556100,1);
  iVar3 = FUN_0045ab60();
  if (iVar3 == 0) {
    DAT_00556ecc = DAT_00556ecc & 0xfffffffd;
  }
  else {
    DAT_00556ecc = DAT_00556ecc | 2;
  }
  if (DAT_00556ec8 == 1) {
    FUN_00424830();
    FUN_00424810();
    FUN_0040e8d0();
  }
  FUN_004293b0();
  if ((DAT_00556ecc & 0x100000) == 0) {
    FUN_004075a0();
  }
  if ((((DAT_00556ecc & 2) != 0) && ((DAT_00556ecc & 0x2000) != 0)) &&
     (FUN_00477010(DAT_005560e0,1), (DAT_00556ecc & 0x20000) == 0)) {
    FUN_004291a0(fVar10,1);
    FUN_00449c00();
    FUN_00473b40(DAT_005560f4,0);
    if (((DAT_005560ec != 0) && (*(int *)(DAT_005560ec + 0x24) != 0)) && (DAT_0055610c != 0)) {
      FUN_00473b40(DAT_0055610c,1);
    }
    FUN_00477150(DAT_005560e0,DAT_005560d8,param_1);
    FUN_00473b40(DAT_005560f4,1);
    FUN_00450780();
  }
  FUN_00477fa0();
  FUN_00465a90();
  FUN_004651a0();
  if ((DAT_00556ec8 == 1) && ((DAT_00556ecc & 0x100000) == 0)) {
    FUN_00435300(DAT_005560d4,DAT_005560dc,fVar10,param_1);
    FUN_00454600(DAT_005560d4,DAT_005560dc,fVar10,param_1);
  }
  FUN_00450780();
  if (((DAT_00556ecc & 2) == 0) || ((DAT_00556ecc & 0x1000) == 0)) {
    iVar3 = 0;
  }
  else {
    iVar3 = 1;
  }
  if (iVar3 != 0) {
    FUN_00426250();
  }
  FUN_0043c2b0(fVar10,iVar3);
  if ((DAT_00556ecc & 0x800) != 0) {
    if ((DAT_00556ecc & 2) == 0) {
      FUN_0045a730(1,param_1);
    }
    FUN_0045a730(0,param_1);
    if ((((DAT_00556ecc & 2) != 0) && ((DAT_00556ecc & 0x2000) != 0)) &&
       ((DAT_00556ecc & 0x20000) == 0)) {
      FUN_0045a730(2,param_1);
    }
    FUN_0045a730(4,param_1);
    FUN_0045a730(9,param_1);
    FUN_0045a730(6,param_1);
    FUN_0045a730(8,param_1);
    FUN_0045d6d0();
    FUN_0045a730(0xb,param_1);
    FUN_0040e7f0();
    if ((DAT_00556ed0 & 0x1000) == 0) {
      FUN_0041b490();
    }
    FUN_0041b930();
    FUN_0045a730(5,param_1);
    fVar9 = param_1;
    fVar1 = param_1;
    if (DAT_005560d4 != 0) {
      fVar9 = *(float *)(DAT_005560d4 + 0x8c);
      fVar1 = (float)(*(int *)(DAT_005560d4 + 0x8c) - *(int *)(DAT_005560d4 + 0x98));
    }
    FUN_0045c0d0(fVar9,fVar1,fVar10);
    FUN_0045c5f0();
    FUN_0045a460(5,DAT_00556128,0x10,0x1d5,1,&DAT_004a2b54,
                 *(int *)(DAT_005560d4 + 0xa0) + *(int *)(DAT_005560d4 + 0xac) * 5);
    FUN_0045a730(7,param_1);
    FUN_00463d20();
    FUN_0041a080();
    FUN_0045a730(10,param_1);
    FUN_00419f30();
  }
  FUN_00418a00();
  FUN_004589a0();
  FUN_00401990();
  FUN_00488c00();
  if ((DAT_00556ecc & 0x1000000) == 0) {
    FUN_00453320();
  }
  FUN_00458be0(DAT_00556140,DAT_005560d4,fVar10,fVar2);
  if (DAT_00505f88 < 2) {
    if ((*(uint *)(DAT_005560d4 + 0x278) != 0) &&
       (*(uint *)(DAT_005560d4 + 0x278) <= *(uint *)(DAT_005560d4 + 0x9c))) {
      FUN_00458670();
    }
    iVar3 = FUN_00437460(0xf,0,0,1);
    if (iVar3 == 0) {
      uVar11 = 0;
      iVar12 = 0;
      iVar3 = FUN_00469f40(2,0);
      if (iVar3 != 0) {
        do {
          iVar3 = FUN_00437460(2,0,uVar11,1);
          iVar12 = iVar12 + iVar3;
          uVar11 = uVar11 + 1;
          uVar6 = FUN_00469f40();
        } while (uVar11 < uVar6);
      }
      if ((iVar12 == 0) && (iVar3 = FUN_00431560(), *(int *)(iVar3 + 0x27c) == 0)) {
        FUN_00458670();
      }
    }
  }
  FUN_0046ad00();
  if (((DAT_00556ed0 & 0x4000) != 0) && (DAT_00556eec != 0)) {
    FUN_0047dcb0(DAT_00556eec,0,&DAT_00556f1c,0);
  }
  if (((DAT_00556ed0 & 0x8000) != 0) && (DAT_00556ef4 != 0)) {
    FUN_0047dcb0(DAT_00556ef4,0,&DAT_00556f24,0);
  }
  if ((((DAT_00556ed0 & 0x40) != 0) && (DAT_00556eec != 0)) &&
     (((DAT_00556ed0 & 0x200) == 0 || (iVar3 = FUN_0041b9b0(), iVar3 != 0)))) {
    FUN_0047dcb0(DAT_00556eec,0,&DAT_00556edc,0);
  }
  if (((DAT_00556ed0 & 0x80) != 0) && (DAT_00556ef0 != 0)) {
    FUN_0047dcb0(DAT_00556ef0,0,&DAT_00556ee4,0);
  }
  if ((((DAT_00556ed0 & 0x100) != 0) && (iVar3 = FUN_00416d00(), iVar3 != 0)) &&
     (iVar3 = FUN_00415370(param_1,1), iVar3 != 0)) {
    DAT_00556ed0 = DAT_00556ed0 & 0xfffffeff;
    iVar3 = FUN_004589a0();
    if (iVar3 == 0) {
      FUN_00425c30(0,0);
    }
    else {
      DAT_00556ecc = DAT_00556ecc | 0x2000000;
    }
  }
  FUN_004230c0();
  if ((DAT_00556ecc & 0x2000000) == 0) {
    FUN_0045cd90();
    if (DAT_00556fa0 != 0) goto LAB_00423edc;
    uVar14 = CONCAT44(DAT_0076ab28,DAT_0076ab24);
  }
  else {
    iVar3 = FUN_00416d00();
    if ((iVar3 != 0) && (iVar3 = FUN_00415370(param_1,0), iVar3 != 0)) {
      DAT_00556ecc = DAT_00556ecc ^ 0x2000000;
      iVar3 = FUN_00416de0();
      if (iVar3 == 0) {
        iVar3 = FUN_004589a0();
        if (iVar3 == 0) {
          FUN_00425c30(0,0);
        }
      }
      else {
        FUN_0041fa80();
        iVar3 = FUN_00431560();
        *(undefined4 *)(iVar3 + 0x27c) = 3;
        FUN_00460220();
        if ((DAT_00556ecc & 8) != 0) {
          DAT_00556ed0 = DAT_00556ed0 | 0x800;
          FUN_004297b0();
        }
      }
      goto LAB_00423edc;
    }
    FUN_0045cd90();
    uVar14 = CONCAT44(DAT_0076ab28,DAT_0076ab24);
  }
  FUN_0045cc70(uVar14);
LAB_00423edc:
  iVar3 = FUN_00416df0();
  if ((iVar3 == 1) || (DAT_00505f88 == 0xb)) {
    FUN_00415220();
  }
  if ((DAT_00556ecc & 0x20000000) != 0) {
    pcVar7 = s_Vertex_004a40a0;
    if ((DAT_00556ecc & 0x40000) == 0) {
      pcVar7 = s_Block_004a4098;
    }
    FUN_0047a180(DAT_0055612c,10,10,s__s_mode_004a4090,pcVar7);
  }
  if (((DAT_00556ecc & 0x40000) != 0) && (DAT_00556188 != 0)) {
    FUN_00450060(*(undefined4 *)(DAT_005560d4 + 4),DAT_00556180,DAT_00556184,&fStack_c);
    FUN_004772b0(DAT_005560dc,&fStack_28,&fStack_c);
    fStack_28 = fStack_28 / fStack_1c;
    fStack_24 = fStack_24 / fStack_1c;
    FUN_0045cc20();
    __ftol();
    uVar4 = __ftol();
    FUN_0045cc70(uVar4);
    DAT_00556ecc = DAT_00556ecc & 0x7fffffff;
  }
  FUN_0046b7c0(DAT_0076ab24,DAT_0076ab28,fVar2);
  if ((DAT_00556ed0 & 0x10) != 0) {
    if (((((DAT_0076ab84._2_1_ != '\0') && (DAT_0076aa04._2_1_ != DAT_0076ab84._2_1_)) ||
         ((DAT_0076ab84._3_1_ != '\0' && (DAT_0076aa04._3_1_ != DAT_0076ab84._3_1_)))) ||
        ((DAT_0076ab88 != '\0' && (DAT_0076aa08 != DAT_0076ab88)))) ||
       ((DAT_0076ab89 != '\0' && (DAT_0076aa09 != DAT_0076ab89)))) {
      _DAT_004de530 = 1.0;
    }
    if (_DAT_0049e2b8 < _DAT_004de530) {
      piVar8 = (int *)FUN_00452580();
      if (fStack_28 != 0.0) {
        piVar8 = (int *)*piVar8;
        if (*piVar8 == 4) {
          iVar3 = piVar8[6] + 0x8c;
        }
        else if (*piVar8 == 1) {
          iVar3 = piVar8[3] + 0x124;
        }
        else {
          iVar3 = 0;
        }
        if (iVar3 != 0) {
          uVar11 = *(uint *)(iVar3 + 8);
          FUN_0047a180(DAT_00556124,0,0,s_Upgrade_____i_i_i_i__004a4078,uVar11 >> 3 & 1,
                       uVar11 >> 2 & 1,uVar11 >> 1 & 1,uVar11 & 1);
        }
      }
      _DAT_004de530 = _DAT_004de530 - _DAT_0049e2c0;
    }
  }
  if (_DAT_004a355c < _DAT_0049e2b8) {
    FUN_00464fa0(5,0);
    fVar13 = (float10)FUN_004798b0(0x42960000,0x42fa0000);
    _DAT_004a355c = (float)fVar13;
  }
  if (_DAT_004a3560 < _DAT_0049e2b8) {
    FUN_00464fa0(6,0);
    fVar13 = (float10)FUN_004798b0(0x437a0000,0x443b8000);
    _DAT_004a3560 = (float)fVar13;
  }
  _DAT_004a355c = _DAT_004a355c - param_1;
  _DAT_004a3560 = _DAT_004a3560 - param_1;
  if ((DAT_00556ed0 & 0x10) != 0) {
    if (DAT_0076abc9 == '\0') {
      fStack_34 = -1.0;
      if (DAT_0076abc8 == '\0') {
        fStack_34 = 0.0;
      }
    }
    else {
      fStack_34 = 1.0;
    }
    if (_DAT_0049e2b8 != fStack_34) {
      _DAT_004de534 = 25.0;
      FUN_0041fb40();
      FUN_00425ce0();
      fVar13 = (float10)FUN_0041fb40();
      if ((float10)_DAT_0049e2d4 < fVar13) {
        FUN_00425ce0();
      }
      fVar13 = (float10)FUN_0041fb40();
      if (fVar13 < (float10)_DAT_0049e2b8) {
        FUN_00425ce0();
      }
    }
    if (_DAT_0049e2b8 < _DAT_004de534) {
      fVar13 = (float10)FUN_0041fb40();
      FUN_0047a180(DAT_00556124,10,0x50,s_Game_Speed__0_0f___004a4064,
                   (double)(fVar13 * (float10)_DAT_0049e2d8));
    }
    _DAT_004de534 = _DAT_004de534 - param_1;
  }
  if ((DAT_00556ed0 & 0x10) != 0) {
    if (_DAT_0049e2c8 == DAT_004a3564) {
      fVar13 = (float10)FUN_0047c100();
      DAT_004a3564 = (float)fVar13;
    }
    fVar2 = _DAT_0049e2dc;
    if ((DAT_0076ab8a != '\0') || (fVar2 = _DAT_0049e2e0, DAT_0076ab8b != '\0')) {
      DAT_004a3564 = DAT_004a3564 - fVar2;
      fVar13 = (float10)FUN_0047c100();
      DAT_004a3564 = (float)fVar13;
      _DAT_004de538 = 25.0;
    }
    if (_DAT_0049e2b8 < _DAT_004de538) {
      FUN_0047a180(DAT_00556124,10,0x50,s_3D_sound_fall_off___f_004a404c,(double)DAT_004a3564);
    }
    _DAT_004de538 = _DAT_004de538 - param_1;
  }
  if ((DAT_00556ecc & 0x1000000) == 0) {
    FUN_00453bb0();
  }
  else {
    FUN_0047a180(DAT_00556124,0,0,s_NoNERPS_004a4044);
  }
  if (_DAT_0049e2b8 < *(float *)(DAT_005560d4 + 0x1fc)) {
    if (*(float *)(DAT_005560d4 + 0x204) - _DAT_0049e2c4 <= *(float *)(DAT_005560d4 + 0x1fc)) {
      if (*(float *)(DAT_005560d4 + 0x204) < *(float *)(DAT_005560d4 + 0x1fc)) {
        if (*(int *)(DAT_005560d4 + 0x1fc) == 0x42c80000) {
          FUN_00419b80(0x13,0,0,0);
        }
        *(undefined4 *)(DAT_005560d4 + 0x204) = *(undefined4 *)(DAT_005560d4 + 0x1fc);
      }
    }
    else {
      if (_DAT_0049e2e4 < *(float *)(DAT_005560d4 + 0x1fc)) {
        FUN_00419b80(0x10,0,0,0);
        uVar4 = 0x15;
      }
      else {
        FUN_00419b80(0x11,0,0,0);
        uVar4 = 0x14;
      }
      FUN_0046ac60(uVar4,1);
      *(undefined4 *)(DAT_005560d4 + 0x204) = *(undefined4 *)(DAT_005560d4 + 0x1fc);
    }
  }
  else {
    FUN_00419b80(0x12,0,0,0);
  }
  if (_DAT_0049e2e4 < *(float *)(DAT_005560d4 + 0x1fc)) {
    if ((DAT_0050080c != 0) && (iVar3 = FUN_00464c70(s_SND_AirBeat_004a4038,&uStack_30), iVar3 != 0)
       ) {
      FUN_00464e20();
      FUN_00465190();
    }
  }
  else {
    if ((DAT_0050080c == 0) && (iVar3 = FUN_00464c70(s_SND_AirBeat_004a4038,&uStack_30), iVar3 != 0)
       ) {
      FUN_00464fa0(uStack_30,1);
    }
    iVar3 = FUN_00464c70(s_SND_AirBeat_004a4038,&uStack_30);
    if (iVar3 != 0) {
      FUN_0047c160();
      iVar3 = __ftol();
      iVar12 = FUN_00465030();
      if (iVar3 != iVar12) {
        FUN_00465010(uStack_30,iVar3);
      }
    }
  }
  FUN_0045c4a0();
  _DAT_00556efc = _DAT_00556efc - fStack_2c;
  return 1;
}


// ==== EngineShutdown @ 00424d00 ====

void EngineShutdown(void)

{
  char *pcVar1;
  uint uVar2;
  int iVar3;
  
  FUN_0044e0a0();
  FUN_0045f750();
  FUN_004297b0(0);
  FUN_00429810(0);
  FUN_0042f0c0();
  FUN_0041a850();
  FUN_00479240(DAT_005560c0);
  FUN_00477220(DAT_005560dc);
  FUN_00472a40(DAT_005560d8);
  FUN_00435dc8(DAT_005560e4);
  FUN_00435dc8(DAT_005560e8);
  FUN_00435dc8(DAT_005560ec);
  uVar2 = 0;
  if (DAT_0055627c != 0) {
    iVar3 = 0;
    do {
      FUN_0046ce10(DAT_005561d4 + iVar3);
      FUN_0048db80(*(undefined4 *)(DAT_005561e8 + uVar2 * 4));
      uVar2 = uVar2 + 1;
      iVar3 = iVar3 + 0x1ec;
    } while (uVar2 < DAT_0055627c);
  }
  FUN_0048db80(DAT_005561e8);
  FUN_0048db80(DAT_00556254);
  FUN_0048db80(DAT_005561d4);
  uVar2 = 0;
  if (DAT_00556280 != 0) {
    iVar3 = 0;
    do {
      FUN_00406ec0(DAT_005561d8 + iVar3);
      FUN_0048db80(*(undefined4 *)(DAT_005561ec + uVar2 * 4));
      uVar2 = uVar2 + 1;
      iVar3 = iVar3 + 0x74;
    } while (uVar2 < DAT_00556280);
  }
  FUN_0048db80(DAT_005561ec);
  FUN_0048db80(DAT_00556258);
  FUN_0048db80(DAT_005561d8);
  uVar2 = 0;
  if (DAT_00556284 != 0) {
    iVar3 = 0;
    do {
      FUN_00406ec0(DAT_005561dc + iVar3);
      FUN_0048db80(*(undefined4 *)(DAT_005561f0 + uVar2 * 4));
      uVar2 = uVar2 + 1;
      iVar3 = iVar3 + 0x74;
    } while (uVar2 < DAT_00556284);
  }
  FUN_0048db80(DAT_005561f0);
  FUN_0048db80(DAT_0055625c);
  FUN_0048db80(DAT_005561dc);
  uVar2 = 0;
  if (DAT_0055628c != 0) {
    iVar3 = 0;
    do {
      FUN_0046c0f0(DAT_005561e4 + iVar3);
      FUN_0048db80(*(undefined4 *)(DAT_005561f8 + uVar2 * 4));
      uVar2 = uVar2 + 1;
      iVar3 = iVar3 + 0x10;
    } while (uVar2 < DAT_0055628c);
  }
  FUN_0048db80(DAT_005561f8);
  FUN_0048db80(DAT_00556264);
  FUN_0048db80(DAT_005561e4);
  uVar2 = 0;
  if (DAT_00556288 != 0) {
    iVar3 = 0;
    do {
      FUN_00408940(DAT_005561e0 + iVar3);
      FUN_0048db80(*(undefined4 *)(DAT_005561f4 + uVar2 * 4));
      uVar2 = uVar2 + 1;
      iVar3 = iVar3 + 0x14c;
    } while (uVar2 < DAT_00556288);
  }
  FUN_0048db80(DAT_005561f4);
  FUN_0048db80(DAT_00556260);
  FUN_0048db80(DAT_005561e0);
  FUN_0048db80(DAT_0055618c);
  FUN_0048db80(DAT_00556190);
  FUN_0048db80(DAT_00556194);
  FUN_0048db80(DAT_00556198);
  FUN_0048db80(DAT_0055619c);
  FUN_0048db80(DAT_005561a0);
  FUN_0048db80(DAT_005561a4);
  FUN_0048db80(DAT_005561a8);
  FUN_0048db80(DAT_005561ac);
  FUN_00485190(DAT_00556140);
  FUN_0047a550(DAT_00556124);
  FUN_0047a550(DAT_00556128);
  FUN_0040c6c0();
  FUN_004020c0();
  FUN_00437050();
  FUN_00472800();
  FUN_00476d80();
  if (DAT_00505f88 == 10) {
    iVar3 = FUN_00488b90();
    pcVar1 = s_LoaderProfile_txt_004a3acc;
    if (iVar3 == 0) {
      pcVar1 = s_LoaderProfileNoSound_txt_004a3ab0;
    }
  }
  else {
    pcVar1 = (char *)0x0;
  }
  FUN_0044dd40(pcVar1);
  FUN_00484c90();
  FUN_0047d430();
  FUN_00487760();
  FUN_0047a800();
  FUN_0047ce20(0,0);
  return;
}


// ==== EngineShutdownAlt @ 00424cf0 ====

void EngineShutdownAlt(void)

{
  FUN_004250a0();
  return;
}


// ==== MainSimStep @ 004250c0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
MainSimStep(undefined4 param_1,float param_2,undefined4 *param_3,undefined4 *param_4,
           undefined4 *param_5)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  int *piVar4;
  undefined4 uVar5;
  int iVar6;
  uint uVar7;
  undefined **ppuVar8;
  undefined4 uVar9;
  bool bVar10;
  float10 fVar11;
  float local_14;
  float local_10;
  undefined1 local_c [8];
  float local_4;
  
  if ((((((DAT_00556ed0 & 0x20000) != 0) && (DAT_0076ab9c == '\0')) && (DAT_0076aa1c != '\0')) &&
      (((DAT_00556ec8 == 1 && ((DAT_00556ecc & 0x100000) == 0)) &&
       ((piVar4 = (int *)FUN_004525c0(), piVar4 != (int *)0x0 && (*piVar4 == 2)))))) &&
     ((piVar4[0xfc] != 0 || (piVar4[0xcc] != 0)))) {
    uVar5 = FUN_0044a200(piVar4);
    FUN_004754c0(uVar5,0,local_c);
    fVar11 = (float10)FUN_00469d50(piVar4);
    local_4 = (float)((float10)local_4 - fVar11 * (float10)_DAT_0049e2f4);
    FUN_004772d0(DAT_005560dc,&local_14,local_c);
    if ((_DAT_0049e300 <= local_14) &&
       (((_DAT_0049e300 <= local_10 && (local_14 <= (float)DAT_00505924 - _DAT_0049e300)) &&
        (local_10 <= (float)DAT_00505928 - _DAT_0049e304)))) {
      FUN_00425c30(0,1);
      if (piVar4[2] == 0) {
        FUN_00437880(piVar4,s_Object_004a4128);
        *(undefined1 *)piVar4[2] = 0;
      }
      DAT_00556fa0 = piVar4[2];
      _DAT_00556fa4 = local_14;
      _DAT_00556fa8 = local_10;
    }
  }
  if (DAT_0076ab81 != '\0') {
    if ((((DAT_00556ed0 & 0x100) == 0) && (iVar6 = FUN_004191a0(), iVar6 == 0)) &&
       ((iVar6 = FUN_004589a0(), iVar6 == 0 && ((DAT_00556ecc & 0x8000000) == 0)))) {
      if ((DAT_00556ed0 & 0x10) != 0) {
        if (DAT_0076abb9 == '\0') {
          if (DAT_0076ab9c != '\0') {
            return 0;
          }
        }
        else {
          FUN_0047a800();
          FUN_004250a0();
        }
      }
      FUN_00425c30(0,1);
      DAT_00556ecc = DAT_00556ecc | 0x2000000;
    }
    else if (((DAT_0076ab81 != '\0') && ((DAT_00556ed0 & 0x100) == 0)) &&
            ((iVar6 = FUN_004191a0(), iVar6 == 0 &&
             ((iVar6 = FUN_004589a0(), iVar6 == 0 && ((DAT_00556ecc & 0x8000000) == 0)))))) {
      FUN_0047a800();
      FUN_004250a0();
    }
  }
  if ((DAT_00556ed0 & 2) != 0) {
    iVar6 = FUN_00416d00();
    if ((iVar6 == 0) || (DAT_0076abb9 != '\0')) {
      FUN_0047a800();
      FUN_004250a0();
    }
    else {
      if (DAT_0076ab9c != '\0') {
        return 0;
      }
      iVar6 = FUN_00435940();
      if (iVar6 == 0) {
        return 0;
      }
    }
  }
  puVar3 = param_5;
  puVar2 = param_4;
  puVar1 = param_3;
  *param_3 = 0;
  *param_4 = 0;
  *param_5 = 0;
  if ((DAT_00556ecc & 0x8100000) == 0) {
    FUN_0041c440();
    if ((DAT_0076abb9 != '\0') && (DAT_0076aa39 != DAT_0076abb9)) {
      FUN_00407340();
    }
    if ((DAT_0076abaa != '\0') || (DAT_0076abb6 != '\0')) {
      *puVar3 = 1;
    }
    if ((DAT_00556ec8 == 1) && (uVar7 = FUN_00456240(0), (uVar7 & 0x1000) == 0)) {
      if (DAT_0076ac50 != '\0') {
        FUN_00436900(DAT_005560e4,param_2 * _DAT_0049e308);
      }
      if (DAT_0076ac48 != '\0') {
        FUN_00436900(DAT_005560e4,param_2 * _DAT_0049e30c);
      }
      if (DAT_0076ac4b != '\0') {
        FUN_004369f4(DAT_005560e4,param_2 * _DAT_0049e308);
      }
      if (DAT_0076ac4d != '\0') {
        FUN_004369f4(DAT_005560e4,param_2 * _DAT_0049e30c);
      }
      if (DAT_0076ab8c != '\0') {
        FUN_00436aa5(DAT_005560e4,param_2 * _DAT_0049e2d4);
      }
      if (DAT_0076ab8d != '\0') {
        FUN_00436aa5(DAT_005560e4,param_2 * _DAT_0049e310);
      }
    }
    if ((DAT_00556ed0 & 0x10) != 0) {
      if (DAT_0076abc7 != '\0') {
        FUN_00425ce0(0x40400000);
      }
      if (DAT_0076ab94 != '\0') {
        *puVar1 = 1;
      }
      if (DAT_0076ab93 != '\0') {
        *puVar2 = 1;
      }
      if (((DAT_0076aba6 != '\0') && (DAT_0076aa26 != DAT_0076aba6)) &&
         (iVar6 = FUN_00435940(), iVar6 == 0)) {
        return 0;
      }
      if (((DAT_0076ab9d != '\0') && (DAT_0076aba1 != '\0')) && (DAT_0076aa21 != DAT_0076aba1)) {
        if ((DAT_00556ecc & 0x40) == 0) {
          DAT_00556ecc = DAT_00556ecc | 0x40;
        }
        else {
          DAT_00556ecc = DAT_00556ecc & 0xffffffbf;
        }
        ppuVar8 = (undefined **)&DAT_004a4124;
        if ((DAT_00556ecc & 0x40) == 0) {
          ppuVar8 = &PTR_LAB_00464649_6_004a4120;
        }
        FUN_00485230(DAT_00556140,s_Frame_Rate_Monitor___s_004a4108,ppuVar8);
      }
      if ((((DAT_00556ecc & 2) != 0) && (DAT_0076ab8f != '\0')) && (DAT_0076aa0f != DAT_0076ab8f)) {
        bVar10 = (DAT_00556ecc & 0x2000) == 0;
        if (bVar10) {
          DAT_00556ecc = DAT_00556ecc & 0xffffefff | 0x2000;
          FUN_0045aac0(0,2,1);
        }
        else {
          DAT_00556ecc = DAT_00556ecc & 0xffffdfff | 0x1000;
          FUN_0045aac0(0,2,0);
        }
        FUN_0045aac0(0,3,bVar10);
      }
      if (DAT_0076ab9d != '\0') {
        if ((DAT_0076aba2 != '\0') && (DAT_0076aa22 != DAT_0076aba2)) {
          if ((DAT_00556ecc & 0x80) == 0) {
            DAT_00556ecc = DAT_00556ecc | 0x80;
          }
          else {
            DAT_00556ecc = DAT_00556ecc & 0xffffff7f;
          }
          ppuVar8 = (undefined **)&DAT_004a4124;
          if ((DAT_00556ecc & 0x80) == 0) {
            ppuVar8 = &PTR_LAB_00464649_6_004a4120;
          }
          FUN_00485230(DAT_00556140,s_Memory_Monitor___s_004a40f4,ppuVar8);
        }
        if (((DAT_0076ab9d != '\0') && (DAT_0076ab9c != '\0')) && (DAT_0076aa1c != DAT_0076ab9c)) {
          if ((DAT_00556ecc & 0x40000000) == 0) {
            DAT_00556ecc = DAT_00556ecc | 0x40000000;
          }
          else {
            DAT_00556ecc = DAT_00556ecc & 0xbfffffff;
          }
        }
      }
    }
    if ((DAT_00556ec8 == 0) && ((*(byte *)(DAT_00556160 + 0x3e4) & 1) == 0)) {
      if (DAT_0076ac48 != '\0') {
        FUN_0044a310(DAT_00556160,1,0,0);
      }
      if (DAT_0076ac50 != '\0') {
        FUN_0044a310(DAT_00556160,0xffffffff,0,0);
      }
      if (DAT_0076ac4b != '\0') {
        FUN_0044a310(DAT_00556160,0,0,0xbd4ccccd);
      }
      if (DAT_0076ac4d != '\0') {
        FUN_0044a310(DAT_00556160,0,0,0x3d4ccccd);
      }
      if (DAT_0076abac != '\0') {
        FUN_0044a310(DAT_00556160,0,0xffffffff,0);
      }
      if (DAT_0076abad != '\0') {
        FUN_0044a310(DAT_00556160,0,1,0);
      }
    }
    if ((DAT_00556ed0 & 0x10) != 0) {
      if ((DAT_0076ab9f != '\0') && (DAT_0076aa1f != DAT_0076ab9f)) {
        if ((DAT_00556ecc & 0x10) == 0) {
          FUN_00429810(1);
          FUN_00464fa0(7,1);
        }
        else {
          FUN_00429810(0);
        }
        ppuVar8 = (undefined **)&DAT_004a4124;
        if ((DAT_00556ecc & 0x10) == 0) {
          ppuVar8 = &PTR_LAB_00464649_6_004a4120;
        }
        FUN_00485230(DAT_00556140,s_Sound_Effects___s_004a40e0,ppuVar8);
      }
      if ((DAT_0076abb2 != '\0') && (DAT_0076aa32 != DAT_0076abb2)) {
        FUN_004297b0((DAT_00556ecc & 8) == 0);
        ppuVar8 = (undefined **)&DAT_004a4124;
        if ((DAT_00556ecc & 8) == 0) {
          ppuVar8 = &PTR_LAB_00464649_6_004a4120;
        }
        FUN_00485230(DAT_00556140,s_Music___s_004a40d4,ppuVar8);
      }
    }
    if ((((DAT_00556ed0 & 0x20) != 0) && (DAT_0076abd2 != '\0')) && (DAT_0076aa52 != DAT_0076abd2))
    {
      FUN_00435d91(DAT_005560e4,0);
      FUN_00464720(DAT_005563b4,1);
    }
    if ((((DAT_00556ed0 & 0x10) != 0) && (DAT_0076abac != '\0')) && (DAT_0076aa2c != DAT_0076abac))
    {
      FUN_00435f32(DAT_005560e4,0x40a00000,0x41c80000);
      iVar6 = FUN_004525c0();
      if (iVar6 != 0) {
        iVar6 = FUN_004525c0();
        *(uint *)(iVar6 + 0x3e0) = *(uint *)(iVar6 + 0x3e0) | 0x40000000;
        uVar5 = FUN_004525c0(0x23,0);
        FUN_004409e0(uVar5);
        uVar5 = FUN_004525c0();
        FUN_00447c40(uVar5);
        uVar5 = FUN_004525c0();
        FUN_00403f30(uVar5);
      }
    }
  }
  uVar5 = 0;
  param_2 = 0.0;
  if ((DAT_004de540 != 0) && (DAT_0076ab3c == 0)) {
    uVar5 = 1;
  }
  if ((DAT_0076abb9 == '\0') || (DAT_0076aa39 == DAT_0076abb9)) {
    uVar9 = 0;
  }
  else {
    uVar9 = 1;
  }
  iVar6 = FUN_004589c0(uVar9,uVar5,&param_2);
  if ((((iVar6 == 0) && (iVar6 = FUN_004191a0(), iVar6 == 0)) &&
      (iVar6 = FUN_004589a0(), iVar6 == 0)) &&
     (((DAT_00556ecc & 0x8000000) == 0 && ((DAT_00556ed0 & 0x100) == 0)))) {
    if ((DAT_0076ab99 != '\0') && (DAT_0076aa19 != DAT_0076ab99)) {
      FUN_00425c30(1,0);
      DAT_00556ecc = DAT_00556ecc ^ 0x2000000;
    }
    if (DAT_00505808 == 0) {
      uVar5 = FUN_00478f50(DAT_005560c4,&DAT_004a24e4,s_LoseFocusAndPause_004a40c0,0);
      iVar6 = FUN_004790d0(DAT_005560c0,uVar5);
      if (((iVar6 == 1) && ((DAT_00556ecc & 0x8000000) == 0)) &&
         (FUN_00425c30(0,1), (DAT_00556ecc & 0x2000000) == 0)) {
        DAT_00556ecc = DAT_00556ecc | 0x2000000;
      }
    }
  }
  DAT_004de540 = DAT_0076ab3c;
  if (param_2 != 0.0) {
    return 0;
  }
  if ((DAT_00556ed0 & 0x10) != 0) {
    if (DAT_0076abaa == '\0') {
      if (DAT_0076ab84._2_1_ != '\0') {
        DAT_004a3568 = DAT_004a3568 - _DAT_0049e308;
        FUN_00473460(DAT_00556104,DAT_004a3568);
      }
      if (DAT_0076ab84._3_1_ != '\0') {
        DAT_004de544 = DAT_004de544 - _DAT_0049e308;
        goto LAB_00425a89;
      }
    }
    else {
      if (DAT_0076ab84._2_1_ != '\0') {
        DAT_004a3568 = DAT_004a3568 - _DAT_0049e30c;
        FUN_00473460(DAT_00556104,DAT_004a3568);
      }
      if (DAT_0076ab84._3_1_ != '\0') {
        DAT_004de544 = DAT_004de544 - _DAT_0049e30c;
LAB_00425a89:
        FUN_00473480(DAT_00556104,DAT_004de544);
      }
    }
    if ((DAT_00556ed0 & 0x10) != 0) goto LAB_00425aa8;
  }
  if ((DAT_00505f84 & 0x8000) == 0) {
    return 1;
  }
LAB_00425aa8:
  if (DAT_0076ab9d != '\0') {
    if ((DAT_0076aba0 != '\0') && (DAT_0076aa20 != DAT_0076aba0)) {
      FUN_00454b80(0);
    }
    if ((DAT_0076ab9f != '\0') && (DAT_0076aa1f != DAT_0076ab9f)) {
      FUN_00454b70(0);
    }
  }
  if (((DAT_0076ac1d != '\0') && (DAT_0076ab9f != '\0')) && (DAT_0076aa1f != DAT_0076ab9f)) {
    FUN_00458670(3);
  }
  if (DAT_0076ab98 != '\0') {
    *(float *)(DAT_005560d4 + 0x1fc) = *(float *)(DAT_005560d4 + 0x1fc) - _DAT_0049e2b0;
  }
  return 1;
}


// ==== ManagerTick @ 0045a730 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void ManagerTick(int param_1,float param_2)

{
  uint uVar1;
  int iVar2;
  float10 fVar3;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  uint local_18;
  undefined4 uStack_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  iVar2 = param_1 * 0x30;
  if (((&DAT_0050013c)[iVar2] & 4) == 0) {
    local_28 = *(float *)(&DAT_00500118 + iVar2);
    local_24 = *(float *)(&DAT_0050011c + iVar2);
  }
  else {
    local_28 = (float)(&DAT_00500120)[param_1 * 0xc];
    local_24 = (float)(&DAT_00500124)[param_1 * 0xc];
  }
  if (param_2 != _DAT_0049e5d8) {
    if (ABS((float)(&DAT_00500128)[param_1 * 0xc] - local_28) <= (float)_DAT_0049e5e0) {
      (&DAT_00500128)[param_1 * 0xc] = local_28;
    }
    else {
      (&DAT_00500128)[param_1 * 0xc] =
           ((_DAT_0049e5e8 / param_2) * (float)(&DAT_00500128)[param_1 * 0xc] + local_28) /
           (_DAT_0049e5e8 / param_2 - _DAT_0049e5ec);
    }
    if (ABS((float)(&DAT_0050012c)[param_1 * 0xc] - local_24) <= (float)_DAT_0049e5e0) {
      (&DAT_0050012c)[param_1 * 0xc] = local_24;
    }
    else {
      (&DAT_0050012c)[param_1 * 0xc] =
           ((_DAT_0049e5e8 / param_2) * (float)(&DAT_0050012c)[param_1 * 0xc] + local_24) /
           (_DAT_0049e5e8 / param_2 - _DAT_0049e5ec);
    }
  }
  fVar3 = (float10)FUN_0048e280((double)(float)(&DAT_00500128)[param_1 * 0xc]);
  local_20 = (float)fVar3;
  local_10 = (float)fVar3;
  fVar3 = (float10)FUN_0048e280((double)(float)(&DAT_0050012c)[param_1 * 0xc]);
  local_1c = (float)fVar3;
  local_c = (float)fVar3;
  if (*(int *)(&DAT_00500114 + iVar2) == 0) {
    FUN_0047dcb0(*(undefined4 *)(&DAT_00500110 + iVar2),0,&local_20,0);
  }
  else if (*(int *)(&DAT_00500114 + iVar2) == 1) {
    local_18 = FUN_004897d0();
    uStack_14 = 0;
    local_8 = (float)local_18;
    local_18 = FUN_004850e0();
    uStack_14 = 0;
    local_4 = (float)local_18;
    FUN_00484070(*(undefined4 *)(&DAT_00500110 + iVar2),&local_10,~DAT_00556ecc >> 0x14 & 1,1);
  }
  if (param_1 == 9) {
    if ((_DAT_00500860 & 0x10) != 0) {
      FUN_0047dcb0(DAT_00500850,0,&DAT_00500830,0);
      _DAT_00500860 = _DAT_00500860 & 0xffffffef;
    }
    if ((_DAT_00500860 & 0x20) != 0) {
      FUN_0047dcb0(DAT_00500854,0,&DAT_00500838,0);
      _DAT_00500860 = _DAT_00500860 & 0xffffffdf;
    }
    if ((_DAT_00500860 & 0x40) != 0) {
      FUN_0047dcb0(DAT_00500858,0,&DAT_00500840,0);
      _DAT_00500860 = _DAT_00500860 & 0xffffffbf;
    }
    if ((_DAT_00500860 & 0x80) != 0) {
      FUN_0047dcb0(DAT_0050085c,0,&DAT_00500848,0);
      _DAT_00500860 = _DAT_00500860 & 0xffffff7f;
    }
  }
  uVar1 = __ftol();
  if (((int)((uVar1 ^ (int)uVar1 >> 0x1f) - ((int)uVar1 >> 0x1f)) < 2) &&
     (uVar1 = __ftol(), (int)((uVar1 ^ (int)uVar1 >> 0x1f) - ((int)uVar1 >> 0x1f)) < 2)) {
    *(uint *)(&DAT_0050013c + iVar2) = *(uint *)(&DAT_0050013c + iVar2) & 0xfffffff7;
  }
  FUN_0045a9c0();
  return;
}


// ==== MessageDispatch @ 00419b80 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void MessageDispatch(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  int iVar6;
  float10 fVar7;
  
  if (((byte)DAT_004dcd48 & 4) != 0) {
    uVar2 = FUN_00456240(0);
    if ((uVar2 & 0x100) == 0) {
      if (((((byte)DAT_004dcd48 & 8) != 0) &&
          ((*(uint *)(&DAT_004dc708 + param_1 * 0x14) & 0x20000) != 0)) &&
         ((float)(&DAT_004dc704)[param_1 * 5] <= _DAT_0049e264)) {
        (&DAT_004dc704)[param_1 * 5] = 0x41c80000;
        if (((param_1 == 10) || (param_1 == 0xb)) || (param_1 == 0xc)) {
          _DAT_004dc7cc = 0x41c80000;
          _DAT_004dc7e0 = 0x41c80000;
          _DAT_004dc7f4 = 0x41c80000;
          _DAT_004dc9fc = 0x41c80000;
        }
        FUN_00464ef0(*(undefined4 *)(&DAT_004dc700 + param_1 * 0x14),0);
      }
      iVar3 = FUN_004199f0(param_1,param_3,param_4);
      if (iVar3 == 0) {
        iVar3 = FUN_00419b00(param_1);
        if (iVar3 != 0) {
          iVar6 = param_1 * 0x14;
          iVar3 = *(int *)(&DAT_004dc6fc + iVar6);
          if (((((byte)DAT_004dcd48 & 8) != 0) &&
              ((*(uint *)(&DAT_004dc708 + iVar6) & 0x20000) == 0)) &&
             ((float)(&DAT_004dc704)[param_1 * 5] <= _DAT_0049e264)) {
            (&DAT_004dc704)[param_1 * 5] = 0x41c80000;
            if ((((param_1 == 10) || (param_1 == 0xb)) || (param_1 == 0xc)) || (param_1 == 0x26)) {
              _DAT_004dc7cc = 0x41c80000;
              _DAT_004dc7e0 = 0x41c80000;
              _DAT_004dc7f4 = 0x41c80000;
              _DAT_004dc9fc = 0x41c80000;
            }
            FUN_00464ef0(*(undefined4 *)(&DAT_004dc700 + iVar6),0);
          }
          if (DAT_004dcd38 != 0x28) {
            iVar4 = FUN_00419b20(param_1);
            iVar1 = DAT_004dcd38;
            if (iVar4 == -1) {
              iVar4 = DAT_004dcd38 * 5;
              DAT_004dcd38 = DAT_004dcd38 + 1;
              (&DAT_004dca24)[iVar4] = -(float)*(int *)(iVar3 + 4);
              fVar7 = (float10)FUN_00419b50();
              (&DAT_004dca28)[iVar1 * 5] = (float)fVar7;
              iVar4 = iVar1;
            }
            (&DAT_004dca20)[iVar4 * 5] = param_1;
            uVar5 = FUN_00419a80(param_4,param_3,param_2);
            FUN_004196f0(iVar4,uVar5);
            uVar2 = FUN_00401bf0();
            if (((uVar2 < 5) && (((byte)DAT_004dcd48 & 0x10) != 0)) &&
               ((*(uint *)(&DAT_004dc708 + iVar6) & 0x10000) != 0)) {
              fVar7 = (float10)FUN_0041fb40();
              if ((float10)_DAT_0049e25c < fVar7) {
                FUN_00425ce0(0x3f800000);
              }
            }
          }
        }
      }
    }
  }
  return;
}


// ==== ViewContextTick @ 00435f5c ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void ViewContextTick(int *param_1,int param_2,float param_3,float param_4)

{
  float fVar1;
  int iVar2;
  undefined4 uVar3;
  float10 fVar4;
  float fVar5;
  undefined1 local_e0 [12];
  undefined1 local_d4 [12];
  undefined1 local_c8 [12];
  float local_bc;
  undefined4 local_b8;
  undefined4 local_b4;
  undefined4 local_b0;
  undefined1 local_ac [12];
  undefined1 local_a0 [12];
  float local_94;
  float local_90;
  float local_8c;
  float local_88;
  undefined4 local_84;
  undefined4 local_80;
  float local_7c;
  int local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  float local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined1 local_24 [12];
  undefined4 local_18;
  undefined4 local_14;
  uint local_c;
  uint local_8;
  
  FUN_004755e0(param_1[9],1,-(float)param_1[0x2b],-(float)param_1[0x2c],-(float)param_1[0x2d]);
  if (*param_1 == 1) {
    FUN_00401720(param_1 + 6,param_1 + 6,DAT_005560a4);
    FUN_004755e0(param_1[10],2,param_1[6],param_1[7],param_1[8]);
    FUN_004754c0(param_1[10],0,&local_30);
    FUN_00475500(param_1[10],0,local_24,&local_18);
    iVar2 = FUN_0044f6d0(*(undefined4 *)(param_2 + 4),local_30,local_2c,&local_8,&local_c);
    if (iVar2 == 0) {
      FUN_004755e0(param_1[10],2,-(float)param_1[6],-(float)param_1[7],-(float)param_1[8]);
    }
    else {
      if ((local_8 < DAT_005560ac) || (**(int **)(param_2 + 4) - DAT_005560ac <= local_8)) {
        FUN_004755e0(param_1[10],2,-(float)param_1[6],0,0);
      }
      if ((local_c < DAT_005560ac) ||
         (*(int *)(*(int *)(param_2 + 4) + 4) - DAT_005560ac <= local_c)) {
        FUN_004755e0(param_1[10],2,0,-(float)param_1[7],0);
      }
      fVar4 = (float10)FUN_0044f940(*(undefined4 *)(param_2 + 4),local_30,local_2c);
      FUN_00475430(param_1[0xb],0,local_30,local_2c,(float)fVar4);
      FUN_00475470(param_1[0xb],0,local_18,local_14,0,0,0,0xbf800000);
      uVar3 = FUN_00431570(local_30,local_2c);
      fVar4 = (float10)FUN_0044fe70(uVar3);
      local_34 = (float)-fVar4;
      local_38 = (float)param_1[0x24] + local_34;
      FUN_00475430(param_1[9],param_1[0xc],0,0,-local_38);
    }
    goto LAB_00436718;
  }
  if (*param_1 != 3) {
    if (*param_1 == 2) {
      if (param_1[1] == 0) {
        FUN_0041f940();
      }
      else {
        local_78 = FUN_0044a200();
        if (local_78 != 0) {
          local_88 = (float)param_1[2] * param_4;
          FUN_0041f940();
          FUN_004754c0(param_1[9],0,local_a0);
          FUN_004754c0(local_78,0,&local_84);
          local_7c = local_7c - (float)param_1[3] / _DAT_0049e398;
          fVar5 = (float)param_1[0x24];
          param_1[0x24] = param_1[3];
          FUN_00475430(param_1[9],param_1[0xc],0,0,-(float)param_1[0x24]);
          FUN_004754c0(param_1[10],0,&local_b8);
          FUN_00475430(param_1[10],0,local_84,local_80,local_7c);
          FUN_004754c0(param_1[9],0,local_e0);
          FUN_004016c0(local_c8,local_e0,local_a0);
          fVar4 = (float10)FUN_004013a0();
          local_8c = (float)fVar4;
          if (((float10)local_88 < fVar4) && (local_8c < _DAT_0049e39c)) {
            local_90 = local_8c / local_88;
            FUN_00475500(local_78,0,local_ac,0);
            FUN_00401720(local_d4,local_ac,local_90);
            FUN_00401500(&local_84,&local_84,local_d4);
            FUN_004016c0(local_c8,&local_84,&local_b8);
            fVar4 = (float10)FUN_004013a0();
            local_94 = (float)fVar4;
            fVar4 = (float10)FUN_0048e680((double)((float)param_1[3] - fVar5));
            local_bc = (float)fVar4;
            fVar1 = local_94 + local_bc;
            FUN_00401750();
            FUN_00401720(local_c8,local_c8,(local_94 / fVar1) * local_88);
            FUN_00401500(&local_b8,&local_b8,local_c8);
            FUN_00475430(param_1[10],0,local_b8,local_b4,local_b0);
            if (fVar5 <= (float)param_1[3]) {
              if (fVar5 != (float)param_1[3]) {
                param_1[0x24] = (int)((local_bc / fVar1) * local_88 + fVar5);
              }
            }
            else {
              param_1[0x24] = (int)(fVar5 - (local_bc / fVar1) * local_88);
            }
            FUN_00475430(param_1[9],param_1[0xc],0,0,-(float)param_1[0x24]);
          }
          FUN_004369f4(param_1,(float)param_1[4] * param_3);
        }
      }
    }
    goto LAB_00436718;
  }
  if ((param_1[1] == 0) || (param_3 == _DAT_0049e37c)) goto LAB_00436718;
  FUN_004754c0(param_1[9],0,&local_58);
  FUN_00475500(param_1[9],0,&local_4c,&local_68);
  FUN_0044a070(param_1[1],param_1[5],&local_74,&local_68);
  if ((*(byte *)(param_1 + 0x2e) & 0x10) == 0) {
LAB_0043626e:
    local_58 = local_74;
    local_54 = local_70;
    local_50 = local_6c;
    local_4c = local_68;
    local_48 = local_64;
    local_44 = local_60;
  }
  else {
    fVar4 = (float10)FUN_00401420(&local_74,&local_58);
    local_40 = (float)fVar4;
    if ((float10)_DAT_0049e380 < fVar4) goto LAB_0043626e;
    fVar4 = (float10)FUN_004016f0(&local_68,&local_4c);
    local_5c = (float)fVar4;
    if (fVar4 < (float10)_DAT_0049e37c) goto LAB_0043626e;
  }
  local_3c = (_DAT_0049e378 / param_3) * _DAT_0049e384;
  FUN_00401750();
  FUN_00401750();
  FUN_00401720(&local_4c,&local_4c,local_3c);
  FUN_00401500(&local_68,&local_68,&local_4c);
  FUN_00401720(&local_58,&local_58,local_3c);
  FUN_00401500(&local_74,&local_74,&local_58);
  FUN_00401720(&local_74,&local_74,_DAT_0049e378 / (_DAT_0049e378 + local_3c));
  if ((float)_DAT_0049e388 < local_40) {
    FUN_00475430(param_1[9],0,local_74,local_70,local_6c);
  }
  if (local_5c < (float)_DAT_0049e390) {
    FUN_00475470(param_1[9],0,local_68,local_64,local_60,0,0,0xbf800000);
  }
  *(byte *)(param_1 + 0x2e) = *(byte *)(param_1 + 0x2e) | 0x10;
LAB_00436718:
  if ((float)param_1[0x29] <= (float)param_1[0x2a]) {
    param_1[0x2d] = 0;
    param_1[0x2c] = 0;
    param_1[0x2b] = 0;
  }
  else {
    fVar5 = (((float)param_1[0x29] - (float)param_1[0x2a]) / (float)param_1[0x29]) *
            (float)param_1[0x28];
    fVar4 = (float10)FUN_004798b0(0,0x3f800000,fVar5);
    FUN_00479500();
    FUN_00401720(param_1 + 0x2b,param_1 + 0x2b,fVar5 * (float)fVar4);
    param_1[0x2a] = (int)((float)param_1[0x2a] + param_4);
    FUN_004755e0(param_1[9],1,-(float)param_1[0x2b],-(float)param_1[0x2c],-(float)param_1[0x2d]);
  }
  return;
}


// ==== EnginePreTick @ 004231f0 ====

void EnginePreTick(void)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  undefined4 *puVar5;
  char *pcVar6;
  undefined4 *puVar7;
  
  if (DAT_00556fa0 != (char *)0x0) {
    uVar2 = 0xffffffff;
    pcVar6 = DAT_00556fa0;
    do {
      if (uVar2 == 0) break;
      uVar2 = uVar2 - 1;
      cVar1 = *pcVar6;
      pcVar6 = pcVar6 + 1;
    } while (cVar1 != '\0');
    uVar3 = ~uVar2 - 1;
    if ((DAT_0076ab8e == '\0') || (DAT_004de43e == DAT_0076ab8e)) {
      if (((DAT_0076ab9c == '\0') && (DAT_004de44c != '\0')) ||
         ((DAT_0076ab81 == '\0' && (DAT_004de431 != '\0')))) {
        DAT_00556fa0 = (char *)0x0;
        FUN_00425c30(0,0);
      }
      else {
        uVar2 = 0;
        do {
          if ((((uVar3 < 0xb) && ((&DAT_0076ab80)[uVar2] != '\0')) &&
              ((&DAT_004de430)[uVar2] != (&DAT_0076ab80)[uVar2])) &&
             (iVar4 = FUN_00411f00(uVar2), iVar4 != 0)) {
            cVar1 = FUN_0048e380(iVar4);
            DAT_00556fa0[uVar3] = cVar1;
            uVar3 = uVar3 + 1;
            DAT_00556fa0[uVar3] = '\0';
          }
          uVar2 = uVar2 + 1;
        } while (uVar2 < 0x100);
      }
    }
    else if (uVar3 != 0) {
      DAT_00556fa0[~uVar2 - 2] = '\0';
    }
    puVar5 = (undefined4 *)&DAT_0076ab80;
    puVar7 = (undefined4 *)&DAT_004de430;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar7 = puVar7 + 1;
    }
    puVar5 = (undefined4 *)&DAT_0076ab80;
    for (iVar4 = 0x40; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
  }
  return;
}


