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


