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


// ==== QuitPoll @ 004589c0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void QuitPoll(int param_1,int param_2,uint *param_3)

{
  int iVar1;
  uint uVar2;
  
  uVar2 = 4;
  if ((_DAT_004ffbc0 & 1) == 0) {
    if ((_DAT_004ffbc0 & 2) == 0) {
      if ((_DAT_004ffbc0 & 4) != 0) {
        uVar2 = (uint)((byte)(_DAT_004ffbc0 >> 8) & 1 | 2);
      }
    }
    else {
      uVar2 = 1;
    }
  }
  else {
    uVar2 = 0;
  }
  *param_3 = 0;
  if (((param_1 != 0) || (param_2 != 0)) && ((&DAT_004ffdd8)[uVar2] == (&DAT_004ffdf8)[uVar2])) {
    if (((DAT_0076ab24 < 0x82) || (*(int *)(DAT_00556ef0 + 4) + 0x82 <= DAT_0076ab24)) ||
       ((DAT_0076ab28 < 0x13b || (*(int *)(DAT_00556ef0 + 8) + 0x13b <= DAT_0076ab28)))) {
      if ((_DAT_004ffbc0 & 1) == 0) {
        if ((_DAT_004ffbc0 & 6) != 0) {
          iVar1 = FUN_0046a4c0(0x40b);
          if (iVar1 != 0) {
            FUN_004588e0();
            iVar1 = FUN_00435940();
            *param_3 = (uint)(iVar1 == 0);
          }
        }
      }
      else {
        FUN_004588e0();
        FUN_004262e0(0);
        FUN_00426320(0);
        FUN_00425c30(0,0);
        FUN_0047f0d0(DAT_00505924 / 2,DAT_00505928 / 2);
        iVar1 = FUN_00407360();
        if (iVar1 != 0) {
          FUN_00407340();
        }
      }
    }
  }
  if (uVar2 == 4) goto LAB_00458bcc;
  if ((uint)(&DAT_004ffdd8)[uVar2] < (uint)(&DAT_004ffdf8)[uVar2]) {
    if (((DAT_0076ab24 < 0x1d6) || (*(int *)(DAT_00556eec + 4) + 0x1d6 <= DAT_0076ab24)) ||
       ((DAT_0076ab28 < 0x13b || (*(int *)(DAT_00556eec + 8) + 0x13b <= DAT_0076ab28)))) {
LAB_00458b47:
      if (param_1 == 0) goto LAB_00458b60;
      if (param_2 != 0) goto LAB_00458b4f;
    }
    else {
      FUN_0046b770(0x21);
      if (param_2 == 0) goto LAB_00458b47;
LAB_00458b4f:
      FUN_00428800(0);
    }
    (&DAT_004ffdd8)[uVar2] = (&DAT_004ffdd8)[uVar2] + 1;
  }
LAB_00458b60:
  if (((((&DAT_004ffdd8)[uVar2] != 0) && (0x81 < DAT_0076ab24)) &&
      (DAT_0076ab24 < *(int *)(DAT_00556ef0 + 4) + 0x82)) &&
     ((0x13a < DAT_0076ab28 && (DAT_0076ab28 < *(int *)(DAT_00556ef0 + 8) + 0x13b)))) {
    FUN_0046b770(0x22);
    if (param_2 != 0) {
      FUN_00428800(0);
      (&DAT_004ffdd8)[uVar2] = (&DAT_004ffdd8)[uVar2] + -1;
    }
  }
LAB_00458bcc:
  FUN_004589a0();
  return;
}


// ==== SelectedEntity @ 004525c0 ====

uint SelectedEntity(void)

{
  return -(uint)(DAT_004ff298 != 0) & DAT_004fef2c;
}


// ==== TransitionControl @ 00425c30 ====

void TransitionControl(int param_1,uint param_2)

{
  if ((DAT_00556ed0 & 2) == 0) {
    if (param_1 != 0) {
      param_2 = ~DAT_00556ecc >> 0x14 & 1;
    }
    if (param_2 != 0) {
      if ((DAT_00556ecc & 0x100000) == 0) {
        FUN_0047b160();
      }
      DAT_00556ecc = DAT_00556ecc | 0x100000;
      DAT_00556ed0 = DAT_00556ed0 | 0x1000;
      return;
    }
    FUN_00425b40(0);
    FUN_00464fa0(7,1);
    DAT_00556ecc = DAT_00556ecc & 0xffefffff;
    DAT_00556ed0 = DAT_00556ed0 & 0xffffefff;
  }
  return;
}


// ==== EntityRename @ 00437880 ====

void EntityRename(int param_1,char *param_2)

{
  undefined4 uVar1;
  
  if ((param_2 != (char *)0x0) && (*param_2 != '\0')) {
    if (*(int *)(param_1 + 8) == 0) {
      uVar1 = FUN_0048dbd0(0xb);
      *(undefined4 *)(param_1 + 8) = uVar1;
    }
    _strncpy(*(char **)(param_1 + 8),param_2,0xb);
    *(undefined1 *)(*(int *)(param_1 + 8) + 0xb) = 0;
    return;
  }
  if (*(int *)(param_1 + 8) != 0) {
    FUN_0048db80(*(int *)(param_1 + 8));
  }
  *(undefined4 *)(param_1 + 8) = 0;
  return;
}


// ==== KeyToAscii @ 0048e380 ====

uint KeyToAscii(uint param_1)

{
  uint uVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  uint local_8 [2];
  
  uVar1 = param_1;
  if (DAT_00549180 == 0) {
    if ((0x60 < (int)param_1) && ((int)param_1 < 0x7b)) {
      return param_1 - 0x20;
    }
  }
  else {
    if ((int)param_1 < 0x100) {
      if (DAT_004aff74 < 2) {
        uVar2 = (byte)PTR_DAT_004afd68[param_1 * 2] & 2;
      }
      else {
        uVar2 = FUN_0048f1d0(param_1,2);
      }
      if (uVar2 == 0) {
        return uVar1;
      }
    }
    uVar2 = param_1;
    if ((PTR_DAT_004afd68[((int)uVar1 >> 8 & 0xffU) * 2 + 1] & 0x80) == 0) {
      param_1._0_2_ = (ushort)(byte)uVar1;
      uVar3 = 1;
    }
    else {
      param_1._0_2_ = CONCAT11((byte)uVar1,(char)(uVar1 >> 8));
      param_1._3_1_ = SUB41(uVar2,3);
      param_1._0_3_ = (uint3)(ushort)param_1;
      uVar3 = 2;
    }
    iVar4 = FUN_00493ca0(DAT_00549180,0x200,&param_1,uVar3,local_8,3,0);
    if (iVar4 == 0) {
      return uVar1;
    }
    if (iVar4 == 1) {
      return local_8[0] & 0xff;
    }
    param_1 = (local_8[0] >> 8 & 0xff) << 8 | local_8[0] & 0xff;
  }
  return param_1;
}


// ==== KeyContextCheck @ 00411f00 ====

undefined4 KeyContextCheck(int param_1)

{
  return *(undefined4 *)(&DAT_004a1f60 + param_1 * 4);
}


// ==== SystemStateFlags @ 00456240 ====

undefined4 SystemStateFlags(void)

{
  return DAT_004ff980;
}


// ==== MessagePreCheck @ 004199f0 ====

undefined4 MessagePreCheck(int param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  undefined4 local_8;
  undefined4 local_4;
  
  if ((((param_1 != 10) && (param_1 != 0xc)) && (param_1 != 0xb)) && (param_1 != 0x26)) {
    local_4 = param_3;
    uVar2 = 0;
    local_8 = param_2;
    if (DAT_004dcd38 != 0) {
      piVar3 = &DAT_004dca20;
      do {
        if (param_1 == *piVar3) {
          iVar1 = FUN_004196a0(uVar2,&LAB_004199a0,&local_8);
          if (iVar1 != 0) {
            return 1;
          }
        }
        uVar2 = uVar2 + 1;
        piVar3 = piVar3 + 5;
      } while (uVar2 < DAT_004dcd38);
      return 0;
    }
  }
  return 0;
}


// ==== KeepRunning @ 00435940 ====

undefined4 KeepRunning(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  FUN_00431560();
  iVar1 = FUN_00416d50();
  iVar2 = FUN_00416d60();
  FUN_0047b160();
  iVar3 = FUN_00416d00();
  if (iVar3 != 0) {
    FUN_0045fda0();
    FUN_00460360();
    FUN_00461410();
    FUN_004601e0();
  }
  FUN_004295f0(1,0,0);
  FUN_004859b0(DAT_00556140);
  iVar3 = FUN_0042f0c0();
  iVar4 = FUN_00416d00();
  if (iVar4 != 0) {
    if (iVar1 == 0) {
      if (iVar2 == 0) goto LAB_004359e5;
      FUN_00416c80(2);
      iVar1 = FUN_00416dd0();
      if (iVar1 != 0) {
        return 0;
      }
    }
    else {
      FUN_00416c80(1);
      iVar1 = FUN_00416dd0();
      if (iVar1 != 0) {
        return 0;
      }
    }
    iVar3 = FUN_00416d70();
  }
LAB_004359e5:
  FUN_00416940(1,0);
  if (iVar3 == 0) {
    return 0;
  }
  iVar1 = FUN_00429890(iVar3);
  if (iVar1 == 0) {
    return 0;
  }
  FUN_0048db80(iVar3);
  return 1;
}


// ==== SetSystemStateFlags @ 00456550 ====

undefined4 SetSystemStateFlags(undefined4 *param_1)

{
  DAT_004ff980 = *param_1;
  return *param_1;
}


// ==== SelectionRoot @ 004520d0 ====

void SelectionRoot(void)

{
  int iVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  int iVar5;
  
  if (((DAT_00556ecc & 0x100000) == 0) && (uVar4 = 0, DAT_004ff29c != 0)) {
    puVar2 = &DAT_004ff0d0;
    do {
      if ((&DAT_0076ab80)[(byte)(&DAT_004ff0bc)[uVar4]] != '\0') {
        FUN_00452060(puVar2[-2],puVar2[-1],*puVar2,puVar2 + 1);
      }
      uVar4 = uVar4 + 1;
      puVar2 = puVar2 + 5;
    } while (uVar4 < DAT_004ff29c);
  }
  uVar4 = 0;
  if (*(int *)(&DAT_004fef20 + DAT_004fef28 * 4) != 0) {
    do {
      FUN_0045d870(&DAT_004eaf20 + (DAT_004fef28 * 0x800 + uVar4) * 0x14);
      uVar4 = uVar4 + 1;
    } while (uVar4 < *(uint *)(&DAT_004fef20 + DAT_004fef28 * 4));
  }
  iVar5 = *(int *)(&DAT_004fef20 + DAT_004fef28 * 4);
  iVar1 = DAT_004fef28 * 0xa000;
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  DAT_004fef28 = (uint)(DAT_004fef28 == 0);
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  if (iVar5 != 0) {
    puVar3 = (uint *)(&DAT_004eaf28 + iVar1);
    do {
      switch(puVar3[-2]) {
      case 1:
        FUN_004526c0(puVar3[-1],*puVar3);
        break;
      case 3:
        FUN_00452be0();
        break;
      case 4:
        FUN_00452c50(puVar3[-1]);
        break;
      case 5:
      case 6:
        FUN_004035c0(puVar3[-1],puVar3 + 1,*puVar3);
        break;
      case 8:
        FUN_00403270(&DAT_004fef2c,DAT_004ff298,puVar3 + 1,*puVar3);
        break;
      case 9:
        FUN_00452d60(*puVar3);
        break;
      case 10:
        if (DAT_004ff298 != 0) {
          FUN_00425d50(DAT_004fef2c);
        }
        break;
      case 0xb:
        FUN_004295f0(1,0,0);
        break;
      case 0xc:
        FUN_00464fa0(*puVar3,0);
        break;
      case 0x13:
        FUN_00431780(puVar3[1],puVar3[2],*puVar3 & 0xffff,*puVar3 >> 0x10);
        break;
      case 0x15:
        FUN_00439370(puVar3 + 1,0,0,1);
        break;
      case 0x17:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x19:
        FUN_00403610(puVar3[-1],*puVar3);
        break;
      case 0x1b:
        FUN_004394b0(puVar3 + 1,0,0,1);
        break;
      case 0x1d:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x1f:
        FUN_0043aea0();
        break;
      case 0x21:
        FUN_004470d0(puVar3[-1]);
        break;
      case 0x23:
        FUN_00452430(puVar3[-1]);
        break;
      case 0x25:
        FUN_00447210(puVar3[-1],*puVar3);
        break;
      case 0x2b:
        FUN_004036c0(puVar3[-1],*puVar3);
        break;
      case 0x2d:
        FUN_004528c0();
        break;
      case 0x34:
        FUN_00439340(puVar3 + 1,0);
        break;
      case 0x36:
        FUN_00439280(puVar3 + 1);
        break;
      case 0x40:
        FUN_00452cc0();
      }
      puVar3 = puVar3 + 5;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  return;
}


// ==== SelectionClusterA @ 004521d0 ====

void SelectionClusterA(void)

{
  int iVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  int iVar5;
  
  if (((DAT_00556ecc & 0x100000) == 0) && (uVar4 = 0, DAT_004ff29c != 0)) {
    puVar2 = &DAT_004ff0d0;
    do {
      if ((&DAT_0076ab80)[(byte)(&DAT_004ff0bc)[uVar4]] != '\0') {
        FUN_00452060(puVar2[-2],puVar2[-1],*puVar2,puVar2 + 1);
      }
      uVar4 = uVar4 + 1;
      puVar2 = puVar2 + 5;
    } while (uVar4 < DAT_004ff29c);
  }
  uVar4 = 0;
  if (*(int *)(&DAT_004fef20 + DAT_004fef28 * 4) != 0) {
    do {
      FUN_0045d870(&DAT_004eaf20 + (DAT_004fef28 * 0x800 + uVar4) * 0x14);
      uVar4 = uVar4 + 1;
    } while (uVar4 < *(uint *)(&DAT_004fef20 + DAT_004fef28 * 4));
  }
  iVar5 = *(int *)(&DAT_004fef20 + DAT_004fef28 * 4);
  iVar1 = DAT_004fef28 * 0xa000;
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  DAT_004fef28 = (uint)(DAT_004fef28 == 0);
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  if (iVar5 != 0) {
    puVar3 = (uint *)(&DAT_004eaf28 + iVar1);
    do {
      switch(puVar3[-2]) {
      case 1:
        FUN_004526c0(puVar3[-1],*puVar3);
        break;
      case 3:
        FUN_00452be0();
        break;
      case 4:
        FUN_00452c50(puVar3[-1]);
        break;
      case 5:
      case 6:
        FUN_004035c0(puVar3[-1],puVar3 + 1,*puVar3);
        break;
      case 8:
        FUN_00403270(&DAT_004fef2c,DAT_004ff298,puVar3 + 1,*puVar3);
        break;
      case 9:
        FUN_00452d60(*puVar3);
        break;
      case 10:
        if (DAT_004ff298 != 0) {
          FUN_00425d50(DAT_004fef2c);
        }
        break;
      case 0xb:
        FUN_004295f0(1,0,0);
        break;
      case 0xc:
        FUN_00464fa0(*puVar3,0);
        break;
      case 0x13:
        FUN_00431780(puVar3[1],puVar3[2],*puVar3 & 0xffff,*puVar3 >> 0x10);
        break;
      case 0x15:
        FUN_00439370(puVar3 + 1,0,0,1);
        break;
      case 0x17:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x19:
        FUN_00403610(puVar3[-1],*puVar3);
        break;
      case 0x1b:
        FUN_004394b0(puVar3 + 1,0,0,1);
        break;
      case 0x1d:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x1f:
        FUN_0043aea0();
        break;
      case 0x21:
        FUN_004470d0(puVar3[-1]);
        break;
      case 0x23:
        FUN_00452430(puVar3[-1]);
        break;
      case 0x25:
        FUN_00447210(puVar3[-1],*puVar3);
        break;
      case 0x2b:
        FUN_004036c0(puVar3[-1],*puVar3);
        break;
      case 0x2d:
        FUN_004528c0();
        break;
      case 0x34:
        FUN_00439340(puVar3 + 1,0);
        break;
      case 0x36:
        FUN_00439280(puVar3 + 1);
        break;
      case 0x40:
        FUN_00452cc0();
      }
      puVar3 = puVar3 + 5;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  return;
}


// ==== SelectionClusterB @ 00452200 ====

void SelectionClusterB(void)

{
  int iVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  int iVar5;
  
  if (((DAT_00556ecc & 0x100000) == 0) && (uVar4 = 0, DAT_004ff29c != 0)) {
    puVar2 = &DAT_004ff0d0;
    do {
      if ((&DAT_0076ab80)[(byte)(&DAT_004ff0bc)[uVar4]] != '\0') {
        FUN_00452060(puVar2[-2],puVar2[-1],*puVar2,puVar2 + 1);
      }
      uVar4 = uVar4 + 1;
      puVar2 = puVar2 + 5;
    } while (uVar4 < DAT_004ff29c);
  }
  uVar4 = 0;
  if (*(int *)(&DAT_004fef20 + DAT_004fef28 * 4) != 0) {
    do {
      FUN_0045d870(&DAT_004eaf20 + (DAT_004fef28 * 0x800 + uVar4) * 0x14);
      uVar4 = uVar4 + 1;
    } while (uVar4 < *(uint *)(&DAT_004fef20 + DAT_004fef28 * 4));
  }
  iVar5 = *(int *)(&DAT_004fef20 + DAT_004fef28 * 4);
  iVar1 = DAT_004fef28 * 0xa000;
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  DAT_004fef28 = (uint)(DAT_004fef28 == 0);
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  if (iVar5 != 0) {
    puVar3 = (uint *)(&DAT_004eaf28 + iVar1);
    do {
      switch(puVar3[-2]) {
      case 1:
        FUN_004526c0(puVar3[-1],*puVar3);
        break;
      case 3:
        FUN_00452be0();
        break;
      case 4:
        FUN_00452c50(puVar3[-1]);
        break;
      case 5:
      case 6:
        FUN_004035c0(puVar3[-1],puVar3 + 1,*puVar3);
        break;
      case 8:
        FUN_00403270(&DAT_004fef2c,DAT_004ff298,puVar3 + 1,*puVar3);
        break;
      case 9:
        FUN_00452d60(*puVar3);
        break;
      case 10:
        if (DAT_004ff298 != 0) {
          FUN_00425d50(DAT_004fef2c);
        }
        break;
      case 0xb:
        FUN_004295f0(1,0,0);
        break;
      case 0xc:
        FUN_00464fa0(*puVar3,0);
        break;
      case 0x13:
        FUN_00431780(puVar3[1],puVar3[2],*puVar3 & 0xffff,*puVar3 >> 0x10);
        break;
      case 0x15:
        FUN_00439370(puVar3 + 1,0,0,1);
        break;
      case 0x17:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x19:
        FUN_00403610(puVar3[-1],*puVar3);
        break;
      case 0x1b:
        FUN_004394b0(puVar3 + 1,0,0,1);
        break;
      case 0x1d:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x1f:
        FUN_0043aea0();
        break;
      case 0x21:
        FUN_004470d0(puVar3[-1]);
        break;
      case 0x23:
        FUN_00452430(puVar3[-1]);
        break;
      case 0x25:
        FUN_00447210(puVar3[-1],*puVar3);
        break;
      case 0x2b:
        FUN_004036c0(puVar3[-1],*puVar3);
        break;
      case 0x2d:
        FUN_004528c0();
        break;
      case 0x34:
        FUN_00439340(puVar3 + 1,0);
        break;
      case 0x36:
        FUN_00439280(puVar3 + 1);
        break;
      case 0x40:
        FUN_00452cc0();
      }
      puVar3 = puVar3 + 5;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  return;
}


// ==== SelectionGateHelper @ 004521f2 ====

void SelectionGateHelper(void)

{
  int iVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  int iVar5;
  
  if (((DAT_00556ecc & 0x100000) == 0) && (uVar4 = 0, DAT_004ff29c != 0)) {
    puVar2 = &DAT_004ff0d0;
    do {
      if ((&DAT_0076ab80)[(byte)(&DAT_004ff0bc)[uVar4]] != '\0') {
        FUN_00452060(puVar2[-2],puVar2[-1],*puVar2,puVar2 + 1);
      }
      uVar4 = uVar4 + 1;
      puVar2 = puVar2 + 5;
    } while (uVar4 < DAT_004ff29c);
  }
  uVar4 = 0;
  if (*(int *)(&DAT_004fef20 + DAT_004fef28 * 4) != 0) {
    do {
      FUN_0045d870(&DAT_004eaf20 + (DAT_004fef28 * 0x800 + uVar4) * 0x14);
      uVar4 = uVar4 + 1;
    } while (uVar4 < *(uint *)(&DAT_004fef20 + DAT_004fef28 * 4));
  }
  iVar5 = *(int *)(&DAT_004fef20 + DAT_004fef28 * 4);
  iVar1 = DAT_004fef28 * 0xa000;
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  DAT_004fef28 = (uint)(DAT_004fef28 == 0);
  *(undefined4 *)(&DAT_004fef20 + DAT_004fef28 * 4) = 0;
  if (iVar5 != 0) {
    puVar3 = (uint *)(&DAT_004eaf28 + iVar1);
    do {
      switch(puVar3[-2]) {
      case 1:
        FUN_004526c0(puVar3[-1],*puVar3);
        break;
      case 3:
        FUN_00452be0();
        break;
      case 4:
        FUN_00452c50(puVar3[-1]);
        break;
      case 5:
      case 6:
        FUN_004035c0(puVar3[-1],puVar3 + 1,*puVar3);
        break;
      case 8:
        FUN_00403270(&DAT_004fef2c,DAT_004ff298,puVar3 + 1,*puVar3);
        break;
      case 9:
        FUN_00452d60(*puVar3);
        break;
      case 10:
        if (DAT_004ff298 != 0) {
          FUN_00425d50(DAT_004fef2c);
        }
        break;
      case 0xb:
        FUN_004295f0(1,0,0);
        break;
      case 0xc:
        FUN_00464fa0(*puVar3,0);
        break;
      case 0x13:
        FUN_00431780(puVar3[1],puVar3[2],*puVar3 & 0xffff,*puVar3 >> 0x10);
        break;
      case 0x15:
        FUN_00439370(puVar3 + 1,0,0,1);
        break;
      case 0x17:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x19:
        FUN_00403610(puVar3[-1],*puVar3);
        break;
      case 0x1b:
        FUN_004394b0(puVar3 + 1,0,0,1);
        break;
      case 0x1d:
        FUN_00402720(puVar3[-1],0);
        break;
      case 0x1f:
        FUN_0043aea0();
        break;
      case 0x21:
        FUN_004470d0(puVar3[-1]);
        break;
      case 0x23:
        FUN_00452430(puVar3[-1]);
        break;
      case 0x25:
        FUN_00447210(puVar3[-1],*puVar3);
        break;
      case 0x2b:
        FUN_004036c0(puVar3[-1],*puVar3);
        break;
      case 0x2d:
        FUN_004528c0();
        break;
      case 0x34:
        FUN_00439340(puVar3 + 1,0);
        break;
      case 0x36:
        FUN_00439280(puVar3 + 1);
        break;
      case 0x40:
        FUN_00452cc0();
      }
      puVar3 = puVar3 + 5;
      iVar5 = iVar5 + -1;
    } while (iVar5 != 0);
  }
  return;
}


// ==== LevelLoad @ 00429890 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 LevelLoad(undefined4 param_1)

{
  float fVar1;
  float fVar2;
  bool bVar3;
  char cVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 uVar7;
  undefined *puVar8;
  char *pcVar9;
  int iVar10;
  undefined4 uVar11;
  uint uVar12;
  undefined4 *puVar13;
  float *pfVar14;
  uint uVar15;
  float10 fVar16;
  float10 fVar17;
  float local_1c0;
  float local_1bc;
  float local_1b8;
  char *local_1b4;
  int local_1b0;
  int local_1ac;
  int local_1a8;
  int local_1a4;
  int local_1a0;
  int local_19c;
  int local_198;
  int local_194;
  int local_190;
  undefined4 local_18c;
  undefined4 local_188;
  int local_184;
  undefined4 local_180;
  undefined4 local_17c;
  undefined4 local_178;
  undefined4 local_174;
  undefined4 local_170;
  undefined4 local_16c;
  undefined4 local_168;
  undefined4 local_164;
  undefined1 local_160 [4];
  undefined4 local_15c;
  undefined1 local_138 [4];
  undefined4 local_134;
  undefined4 local_130;
  undefined4 local_12c [10];
  undefined1 local_104 [260];
  
  uVar11 = DAT_005560c0;
  FUN_00415990(param_1,1);
  FUN_0046a390();
  FUN_00415150();
  FUN_00418800(0);
  FUN_00409d40(0);
  FUN_0044adc0(0);
  iVar5 = FUN_00417e70();
  if (iVar5 == 0) {
    iVar5 = 0;
  }
  else {
    iVar5 = iVar5 + 0x1c;
  }
  FUN_004370d0(iVar5);
  iVar5 = FUN_004593d0();
  if ((iVar5 == 0) && (iVar5 = FUN_00417e90(), iVar5 != -1)) {
    FUN_0048d9e0(local_104,s__s__i_osf_004a2788,s_Saves_004a2780,iVar5);
    FUN_00459470(local_104);
  }
  puVar6 = (undefined4 *)FUN_0048dbd0(0x284);
  if (puVar6 == (undefined4 *)0x0) goto LAB_0042b268;
  puVar13 = puVar6;
  for (iVar5 = 0xa1; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar13 = 0;
    puVar13 = puVar13 + 1;
  }
  DAT_00556ecc = DAT_00556ecc | 4;
  DAT_005560d4 = puVar6;
  FUN_0044dde0(param_1);
  FUN_0045cf50(uVar11,DAT_005560c4,param_1);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,&DAT_004a45bc,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,&DAT_004a45bc,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  iVar5 = FUN_0048db70(puVar8);
  if (iVar5 != 0) {
    FUN_0048e160(iVar5);
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BoulderAnimation_004a45a8,0);
  pcVar9 = (char *)FUN_004790b0(uVar11,uVar7);
  if ((pcVar9 == (char *)0x0) || (iVar5 = __strcmpi(pcVar9,&DAT_004a45a0), iVar5 == 0)) {
LAB_00429a59:
    puVar6[0x9d] = 0;
  }
  else {
    iVar5 = __strcmpi(pcVar9,&DAT_004a4598);
    if (iVar5 == 0) {
      puVar6[0x9d] = 1;
    }
    else {
      iVar5 = __strcmpi(pcVar9,(char *)&PTR_DAT_004a4594);
      if (iVar5 != 0) goto LAB_00429a59;
      puVar6[0x9d] = 2;
    }
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FallinMultiplier_004a4580,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FallinMultiplier_004a4580,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  DAT_00556f14 = FUN_0048db70(puVar8);
  if (DAT_00556f14 == 0) {
    DAT_00556f14 = 1;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_blocksize_004a4574,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_blocksize_004a4574,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[7] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_digdepth_004a4568,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_digdepth_004a4568,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[8] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_useroof_004a4560,0);
  uVar7 = FUN_004790d0(uVar11,uVar7);
  puVar6[0xb] = uVar7;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_roofheight_004a4554,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_roofheight_004a4554,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[9] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_roughlevel_004a4548,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_roughlevel_004a4548,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[10] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_surfacemap_004a453c,0);
  local_1a0 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_predugmap_004a4530,0);
  local_1a8 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_terrainmap_004a4524,0);
  local_194 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_blockPointersMap_004a4510,0);
  local_198 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_CryOreMap_004a4504,0);
  local_1ac = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeMap_004a44f8,0);
  local_18c = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EmergeMap_004a44ec,0);
  local_184 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_AIMap_004a44e4,0);
  local_188 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_PathMap_004a44dc,0);
  local_1a4 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FallinMap_004a44d0,0);
  local_190 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_textureset_004a44c4,0);
  local_178 = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EmergeTimeOut_004a44b4,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EmergeTimeOut_004a44b4,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar17 = (float10)FUN_0048d980(puVar8);
  fVar16 = (float10)_DAT_0049e2b8;
  puVar6[0x99] = (float)fVar17;
  if (fVar17 == fVar16) {
    puVar6[0x99] = 0x44bb8000;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_olistfile_004a44a8,0);
  local_180 = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_elistfile_004a449c,0);
  FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_selboxheight_004a448c,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_selboxheight_004a448c,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[0xd] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_PTLFile_004a4484,0);
  local_168 = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NERPFile_004a4478,0);
  local_16c = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NERPMessageFile_004a4468,0);
  local_170 = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EmergeCreature_004a4458,0);
  local_1b4 = (char *)FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_SafeCaverns_004a444c,0);
  uVar7 = FUN_004790d0(uVar11,uVar7);
  puVar6[0xc] = uVar7;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_IntroAVI_004a4440,0);
  local_174 = FUN_004790b0(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_StartFP_004a4438,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  puVar6[0x7d] = (uint)(iVar5 == 1);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NoDrain_004a4430,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  puVar6[0x7e] = (uint)(iVar5 == 1);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_OxygenRate_004a4424,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_OxygenRate_004a4424,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[0x80] = (float)(fVar16 * (float10)_DAT_0049e324);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_UpgradeTime_004a4418,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_UpgradeTime_004a4418,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  fVar1 = (float)(fVar16 * (float10)_DAT_0049e2b4);
  pfVar14 = (float *)(puVar6 + 0x84);
  for (iVar5 = 0x14; iVar5 != 0; iVar5 = iVar5 + -1) {
    *pfVar14 = fVar1;
    pfVar14 = pfVar14 + 1;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_MinifigureUpgradeTime_004a4400,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_MinifigureUpgradeTime_004a4400,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  fVar2 = (float)fVar16;
  bVar3 = _DAT_0049e2b8 == fVar2;
  puVar6[0x86] = fVar2;
  if (bVar3) {
    puVar6[0x86] = fVar1;
  }
  else {
    puVar6[0x86] = fVar2 * _DAT_0049e2b4;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_VehicleUpgradeTime_004a43ec,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_VehicleUpgradeTime_004a43ec,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  fVar2 = (float)fVar16;
  bVar3 = _DAT_0049e2b8 == fVar2;
  puVar6[0x85] = fVar2;
  if (bVar3) {
    puVar6[0x85] = fVar1;
  }
  else {
    puVar6[0x85] = fVar2 * _DAT_0049e2b4;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingUpgradeTime_004a43d8,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingUpgradeTime_004a43d8,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  fVar2 = (float)fVar16;
  bVar3 = _DAT_0049e2b8 == fVar2;
  puVar6[0x88] = fVar2;
  if (bVar3) {
    puVar6[0x88] = fVar1;
  }
  else {
    puVar6[0x88] = fVar2 * _DAT_0049e2b4;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_TrainTime_004a43cc,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_TrainTime_004a43cc,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[0x98] = (float)(fVar16 * (float10)_DAT_0049e2b4);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NoMultiSelect_004a43bc,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  if (iVar5 == 1) {
    DAT_00556ed0 = DAT_00556ed0 | 0x2000;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NoAutoEat_004a43b0,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  if (iVar5 == 1) {
    DAT_00556ed0 = DAT_00556ed0 | 0x200000;
  }
  else {
    DAT_00556ed0 = DAT_00556ed0 & 0xffdfffff;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EndGameAVI1_004a43a4,0);
  DAT_00556fb0 = FUN_00479050(uVar11,uVar7);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_EndGameAVI2_004a4398,0);
  DAT_00556fb4 = FUN_00479050(uVar11,uVar7);
  if (DAT_00556fac != 0) {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_AllowRename_004a438c,0);
    iVar5 = FUN_004790d0(uVar11,uVar7);
    if (iVar5 == 1) {
      DAT_00556ed0 = DAT_00556ed0 | 0x20000;
    }
    else {
      DAT_00556ed0 = DAT_00556ed0 & 0xfffdffff;
    }
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_RecallOLObjects_004a437c,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  if (iVar5 == 1) {
    DAT_00556ed0 = DAT_00556ed0 | 0x40000;
  }
  else {
    DAT_00556ed0 = DAT_00556ed0 & 0xfffbffff;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_GenerateSpiders_004a436c,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  if (iVar5 == 1) {
    DAT_00556ed0 = DAT_00556ed0 | 0x80000;
  }
  else {
    DAT_00556ed0 = DAT_00556ed0 & 0xfff7ffff;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_DisableToolTipSound_004a4358,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  if (iVar5 == 1) {
    DAT_00556ed0 = DAT_00556ed0 | 0x100000;
  }
  else {
    DAT_00556ed0 = DAT_00556ed0 & 0xffefffff;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_DisableEndTeleport_004a4344,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  FUN_00458660(iVar5 != 1);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_DragBoxRGB_004a4338,0);
  iVar5 = FUN_00479050(uVar11,uVar7);
  local_19c = iVar5;
  if (iVar5 != 0) {
    iVar10 = FUN_00477440(iVar5,local_12c,&DAT_004a1cd4);
    if (iVar10 == 3) {
      uVar12 = 0;
      do {
        local_1b0 = FUN_0048db70(*(undefined4 *)((int)local_12c + uVar12));
        uVar15 = uVar12 + 4;
        *(float *)((int)&DAT_00556f2c + uVar12) = (float)local_1b0 * _DAT_0049e328;
        iVar5 = local_19c;
        uVar12 = uVar15;
      } while (uVar15 < 0xc);
    }
    FUN_0048db80(iVar5);
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingTolerance_004a4324,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingTolerance_004a4324,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  puVar6[0x82] = (float)fVar16;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingMaxVariation_004a430c,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_BuildingMaxVariation_004a430c,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  bVar3 = _DAT_0049e2b8 == (float)puVar6[0x82];
  puVar6[0x83] = (float)fVar16;
  if (bVar3) {
    puVar6[0x82] = 0x40800000;
  }
  if ((float10)_DAT_0049e2b8 == fVar16) {
    puVar6[0x83] = 0x40c00000;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_MaxStolen_004a4300,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_MaxStolen_004a4300,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  uVar7 = FUN_0048db70(puVar8);
  puVar6[0x9e] = uVar7;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,&DAT_004a42f8,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar6[0x9b] = 0x14;
  }
  else {
    iVar5 = FUN_0042e8b0(iVar5,&local_1b0,puVar6 + 0x9b,0);
    if (iVar5 != 0) {
      uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_SlugTime_004a42ec,0);
      iVar5 = FUN_004790b0(uVar11,uVar7);
      if (iVar5 == 0) {
        puVar8 = &DAT_004b8a14;
      }
      else {
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_SlugTime_004a42ec,0);
        puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
      }
      fVar16 = (float10)FUN_0048d980(puVar8);
      bVar3 = _DAT_0049e2b8 == (float)fVar16;
      puVar6[0x9a] = (float)fVar16;
      if (bVar3) {
        puVar6[0x9a] = 0x42700000;
      }
      puVar6[0x9a] = (float)puVar6[0x9a] * _DAT_0049e2b4;
      uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialSlugTime_004a42dc,0);
      iVar5 = FUN_004790b0(uVar11,uVar7);
      if (iVar5 == 0) {
        puVar8 = &DAT_004b8a14;
      }
      else {
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialSlugTime_004a42dc,0);
        puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
      }
      fVar16 = (float10)FUN_0048d980(puVar8);
      if ((float10)_DAT_0049e2b8 == fVar16) {
        fVar16 = (float10)_DAT_0049e32c;
      }
      _DAT_00556ed8 = (float)(fVar16 * (float10)_DAT_0049e2b4);
    }
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_RockFallStyle_004a42cc,0);
  uVar7 = FUN_004790b0(uVar11,uVar7);
  FUN_0040bf70(uVar7,&local_164);
  FUN_0040bfc0(local_164);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_noFallins_004a42c0,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  DAT_00556ec0 = (uint)(iVar5 != 1);
  FUN_0045c040(uVar11,DAT_005560c4,param_1);
  FUN_0042e8b0(local_1b4,&local_1b4,puVar6 + 0x2c,0);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FogColourRGB_004a42b0,0,&DAT_0055640c,&DAT_00556410,
                       &DAT_00556414);
  iVar5 = FUN_00479170(DAT_005560c0,uVar7);
  if (iVar5 == 0) {
    DAT_00556ecc = DAT_00556ecc & 0xffff7fff;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_HighFogColourRGB_004a429c,0,&DAT_00556418,
                         &DAT_0055641c,&DAT_00556420);
    iVar5 = FUN_00479170(DAT_005560c0,uVar7);
    if (iVar5 == 0) {
      DAT_00556ecc = DAT_00556ecc & 0xfffeffff;
    }
    else {
      DAT_00556ecc = DAT_00556ecc | 0x10000;
      _DAT_00556424 = 0x3f800000;
      uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FogRate_004a4294,0);
      iVar5 = FUN_004790b0(uVar11,uVar7);
      if (iVar5 == 0) {
        puVar8 = &DAT_004b8a14;
      }
      else {
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FogRate_004a4294,0);
        puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
      }
      fVar16 = (float10)FUN_0048d980(puVar8);
      if ((float10)_DAT_0049e2b8 == fVar16) {
        _DAT_00556ef8 = _DAT_0049e32c * _DAT_0049e2b4;
      }
      else {
        _DAT_00556ef8 = (float)fVar16 * _DAT_0049e2b4;
      }
    }
    FUN_00473e10(DAT_0055640c,DAT_00556410,DAT_00556414);
    FUN_00473e70(2);
    FUN_00473ea0(0,(float)DAT_005560d4[7] * _DAT_00556110,0x3b51b717);
    DAT_00556ecc = DAT_00556ecc | 0x8000;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_nextlevel_004a4288,0);
  uVar7 = FUN_00479050(uVar11,uVar7);
  puVar6[0x2d] = uVar7;
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_FullName_004a24ac,0);
  local_1b4 = (char *)FUN_00479050(uVar11,uVar7);
  puVar6[0x9c] = local_1b4;
  if (local_1b4 == (char *)0x0) {
    puVar6[0x9c] = 0;
  }
  else {
    iVar5 = -1;
    uVar12 = 0;
    pcVar9 = local_1b4;
    do {
      if (iVar5 == 0) break;
      iVar5 = iVar5 + -1;
      cVar4 = *pcVar9;
      pcVar9 = pcVar9 + 1;
    } while (cVar4 != '\0');
    if (iVar5 != -2) {
      do {
        cVar4 = local_1b4[uVar12];
        if (cVar4 == '_') {
          cVar4 = ' ';
        }
        local_1b4[uVar12] = cVar4;
        local_1b4 = (char *)puVar6[0x9c];
        uVar15 = 0xffffffff;
        uVar12 = uVar12 + 1;
        pcVar9 = local_1b4;
        do {
          if (uVar15 == 0) break;
          uVar15 = uVar15 - 1;
          cVar4 = *pcVar9;
          pcVar9 = pcVar9 + 1;
        } while (cVar4 != '\0');
      } while (uVar12 < ~uVar15 - 1);
    }
  }
  FUN_00457d40(uVar11,DAT_005560c4,param_1,puVar6,DAT_00505924,DAT_00505928);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeTriggerTime_004a4274,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeTriggerTime_004a4274,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  local_1bc = (float)fVar16;
  if ((float10)_DAT_0049e2b8 == fVar16) {
    local_1bc = 10.0;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeErodeTime_004a4264,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeErodeTime_004a4264,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  local_1b8 = (float)fVar16;
  if ((float10)_DAT_0049e2b8 == fVar16) {
    local_1b8 = 5.0;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeLockTime_004a4254,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_ErodeLockTime_004a4254,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  fVar16 = (float10)FUN_0048d980(puVar8);
  local_1c0 = (float)fVar16;
  if ((float10)_DAT_0049e2b8 == fVar16) {
    local_1c0 = 60.0;
  }
  FUN_0040e930(local_1bc,local_1b8,local_1c0);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NumberOfLandSlidesTillCaveIn_004a4234,0);
  iVar5 = FUN_004790b0(uVar11,uVar7);
  if (iVar5 == 0) {
    puVar8 = &DAT_004b8a14;
  }
  else {
    uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NumberOfLandSlidesTillCaveIn_004a4234,0);
    puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
  }
  iVar5 = FUN_0048db70(puVar8);
  if (iVar5 == 0) {
    iVar5 = 3;
  }
  FUN_0040f5e0(iVar5);
  uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_DisableStartTeleport_004a421c,0);
  iVar5 = FUN_004790d0(uVar11,uVar7);
  *puVar6 = param_1;
  puVar6[0xa0] = (uint)(iVar5 != 1);
  puVar6[0x81] = 0x42c80000;
  puVar6[0x7f] = 0x42c80000;
  puVar6[0x23] = 0;
  puVar6[0x28] = 0;
  puVar6[0x24] = 0;
  puVar6[0x29] = 0;
  puVar6[0x25] = 0;
  puVar6[0x2a] = 0;
  puVar6[0x9f] = 0;
  DAT_0055644c = 0;
  DAT_00556450 = 0;
  FUN_00452df0(local_16c);
  FUN_00452e70(local_170);
  iVar5 = FUN_0045d7e0(local_168,DAT_005560c4);
  if (iVar5 != 0) {
    local_1bc = 0.0;
    local_1c0 = 0.0;
    local_1b8 = 0.0;
    local_19c = 0;
    local_1b0 = 0;
    if ((local_1a0 != 0) && (iVar5 = FUN_00477440(local_1a0,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      FUN_0048db70(local_15c);
    }
    if ((local_1a8 != 0) && (iVar5 = FUN_00477440(local_1a8,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      local_1b0 = FUN_0048db70(local_15c);
    }
    if ((local_194 != 0) && (iVar5 = FUN_00477440(local_194,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      local_19c = FUN_0048db70(local_15c);
    }
    if ((local_198 != 0) && (iVar5 = FUN_00477440(local_198,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      local_1b8 = (float)FUN_0048db70(local_15c);
    }
    if ((local_1ac != 0) && (iVar5 = FUN_00477440(local_1ac,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      local_1c0 = (float)FUN_0048db70(local_15c);
    }
    if ((local_1a4 != 0) && (iVar5 = FUN_00477440(local_1a4,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      local_1bc = (float)FUN_0048db70(local_15c);
    }
    if ((local_190 != 0) && (iVar5 = FUN_00477440(local_190,local_160,&DAT_004a1cd4), iVar5 == 2)) {
      FUN_0048db70(local_15c);
    }
    iVar5 = local_184;
    if ((local_184 != 0) && (iVar10 = FUN_00477440(local_184,local_160,&DAT_004a1cd4), iVar10 == 2))
    {
      local_1bc = (float)FUN_0048db70(local_15c);
    }
    iVar10 = FUN_0042b510(puVar6,local_1a0,local_1a8,local_1b0,local_194,local_19c,local_198,
                          local_1b8,local_1ac,local_1c0,local_18c,local_1a4,local_1bc,local_178,
                          iVar5,local_188,local_190);
    if (iVar10 != 0) {
      FUN_0048db80(local_1a0);
      FUN_0048db80(local_1a8);
      FUN_0048db80(local_194);
      FUN_0048db80(local_198);
      FUN_0048db80(local_1ac);
      FUN_0048db80(local_18c);
      FUN_0048db80(iVar5);
      FUN_0048db80(local_188);
      FUN_0048db80(local_190);
      if (local_1a4 != 0) {
        FUN_0048db80(local_1a4);
      }
      FUN_0040ce80(puVar6);
      FUN_00466230(puVar6);
      FUN_00458670(0);
      FUN_00458be0(DAT_00556140,DAT_005560d4,0x3f800000,0x3f800000);
      iVar5 = FUN_0042e020(puVar6,local_180);
      if (iVar5 != 0) {
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialCrystals_004a420c,0);
        iVar5 = FUN_004790b0(uVar11,uVar7);
        if (iVar5 == 0) {
          puVar8 = &DAT_004b8a14;
        }
        else {
          uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialCrystals_004a420c,0);
          puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
        }
        local_134 = FUN_0048db70(puVar8);
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialOre_004a4200,0);
        iVar5 = FUN_004790b0(uVar11,uVar7);
        if (iVar5 == 0) {
          puVar8 = &DAT_004b8a14;
        }
        else {
          uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_InitialOre_004a4200,0);
          puVar8 = (undefined *)FUN_004790b0(uVar11,uVar7);
        }
        local_130 = FUN_0048db70(puVar8);
        FUN_004377b0(FUN_0042b2f0,local_138);
        FUN_0044dde0(0);
        local_180 = 0x43af0000;
        local_17c = 0x428c0000;
        FUN_0042efc0(local_174,&local_180);
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_NoGather_004a41f4,0);
        iVar5 = FUN_004790d0(uVar11,uVar7);
        FUN_00402170(iVar5 == 1);
        FUN_00419ae0(1);
        if ((puVar6[0x7d] == 0) || (DAT_004e8448 == 0)) {
          FUN_004295f0(1,0,0);
        }
        else {
          FUN_004295f0(0,DAT_004e8448,1);
          FUN_00452060(1,DAT_004e8448,0,0);
          FUN_0041b2d0(0xf,0);
        }
        FUN_00456b80(puVar6);
        uVar7 = FUN_00478f50(DAT_005560c4,param_1,s_Precreate_004a41e8,0);
        iVar5 = FUN_004790d0(uVar11,uVar7);
        if (iVar5 == 1) {
          puVar6 = local_12c;
          iVar5 = 10;
          do {
            uVar11 = FUN_00437d00(DAT_005561d8,2,0);
            *puVar6 = uVar11;
            puVar6 = puVar6 + 1;
            iVar5 = iVar5 + -1;
          } while (iVar5 != 0);
          puVar6 = local_12c;
          iVar5 = 10;
          do {
            FUN_00437540(*puVar6);
            puVar6 = puVar6 + 1;
            iVar5 = iVar5 + -1;
          } while (iVar5 != 0);
        }
        return 1;
      }
    }
  }
  FUN_0048db80(puVar6);
LAB_0042b268:
  FUN_0048db80(local_1a0);
  FUN_0048db80(local_1a8);
  FUN_0048db80(local_194);
  FUN_0048db80(local_198);
  FUN_0048db80(local_1ac);
  FUN_0048db80(local_18c);
  FUN_0048db80(local_184);
  FUN_0044dde0(0);
  DAT_005560d4 = (undefined4 *)0x0;
  return 0;
}


// ==== PendingLevelName @ 0042f0c0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 PendingLevelName(void)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = DAT_005560d4;
  uVar2 = 0;
  if (DAT_005560d4 != 0) {
    FUN_0048db80(*(undefined4 *)(DAT_005560d4 + 8));
    if (DAT_00556fb0 != 0) {
      FUN_0048db80(DAT_00556fb0);
    }
    if (DAT_00556fb4 != 0) {
      FUN_0048db80(DAT_00556fb4);
    }
    FUN_0048db80(*(undefined4 *)(iVar1 + 0x270));
    DAT_00556ecc = DAT_00556ecc & 0xf7dfffff;
    _DAT_004dee14 = _DAT_004dee14 & 0xffffffbf;
    DAT_00556ed0 = DAT_00556ed0 & 0xffffdffd;
    DAT_00556158 = 0;
    DAT_00556514 = 0;
    DAT_004e8448 = 0;
    DAT_004eabc4 = 0;
    DAT_004eabc8 = 0;
    FUN_00435aa0(0);
    DAT_00556ecc = DAT_00556ecc & 0xefffffff;
    DAT_00556ed0 = DAT_00556ed0 & 0xffffbf3b;
    DAT_00556e8c = 0;
    DAT_00556e90 = 0;
    FUN_00435f32(DAT_005560e4,0,0);
    FUN_00436a4a(DAT_005560e4,0x43480000);
    FUN_00465970();
    FUN_00435a20();
    uVar2 = *(undefined4 *)(iVar1 + 0xb4);
    FUN_0046aa90();
    FUN_004181a0();
    FUN_0040ab30();
    FUN_00456c60();
    FUN_00402100(0);
    FUN_004372a0();
    FUN_004099d0();
    FUN_0040bdc0();
    FUN_00402100(1);
    FUN_00419ae0(0);
    FUN_00462ed0();
    FUN_00435ebb(DAT_005560e8,0,0,0,0,0);
    FUN_00435ef4(DAT_005560ec,0,0);
    FUN_00452600();
    FUN_00453220();
    FUN_0045daf0(*(undefined4 *)(iVar1 + 0x84));
    FUN_0044e670(*(undefined4 *)(iVar1 + 4));
    FUN_0042b480(iVar1);
    FUN_0040b410(*(undefined4 *)(iVar1 + 0x7c));
    FUN_00431530(iVar1);
    FUN_0040c690();
    FUN_0048db80(*(undefined4 *)(iVar1 + 0xb8));
    FUN_0048db80(iVar1);
    FUN_00419f10();
    FUN_0041a920();
    FUN_00425ce0(0x3f800000);
    FUN_0045a270();
    FUN_004011c0();
    FUN_0047b160();
    FUN_0040a3d0();
  }
  DAT_005560d4 = 0;
  return uVar2;
}


// ==== VehicleTypeInit @ 0042cda0 ====

undefined4 VehicleTypeInit(void)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  char *pcVar8;
  char *pcVar9;
  int local_4;
  
  iVar7 = 0;
  DAT_0055627c = 0;
  local_4 = 0;
  uVar2 = FUN_00478f50(DAT_005560c4,s_VehicleTypes_004a4630,0);
  for (iVar3 = FUN_00478ff0(DAT_005560c0,uVar2); iVar3 != 0; iVar3 = FUN_00479020(iVar3)) {
    DAT_0055627c = DAT_0055627c + 1;
  }
  if ((DAT_0055627c != 0) && (DAT_005561d4 = FUN_0048dbd0(DAT_0055627c * 0x1ec), DAT_005561d4 != 0))
  {
    DAT_005561e8 = FUN_0048dbd0(DAT_0055627c * 4);
    if (DAT_005561e8 != 0) {
      uVar2 = FUN_00478f50(DAT_005560c4,s_VehicleTypes_004a4630,0);
      iVar3 = FUN_00478ff0(DAT_005560c0,uVar2);
      while ((iVar3 != 0 &&
             (iVar4 = FUN_0046c6f0(DAT_005561d4 + iVar7,local_4,DAT_005560d8,
                                   *(undefined4 *)(iVar3 + 8),DAT_005560c4), iVar4 != 0))) {
        FUN_0046d2c0(DAT_005561d4 + iVar7,1);
        uVar5 = 0xffffffff;
        pcVar8 = *(char **)(iVar3 + 4);
        do {
          if (uVar5 == 0) break;
          uVar5 = uVar5 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        uVar2 = FUN_0048dbd0(~uVar5);
        iVar7 = iVar7 + 0x1ec;
        *(undefined4 *)(DAT_005561e8 + local_4 * 4) = uVar2;
        uVar5 = 0xffffffff;
        pcVar8 = *(char **)(iVar3 + 4);
        do {
          if (uVar5 == 0) break;
          uVar5 = uVar5 - 1;
          cVar1 = *pcVar8;
          pcVar8 = pcVar8 + 1;
        } while (cVar1 != '\0');
        pcVar8 = *(char **)(iVar3 + 4);
        pcVar9 = *(char **)(DAT_005561e8 + local_4 * 4);
        for (uVar6 = ~uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
          *(undefined4 *)pcVar9 = *(undefined4 *)pcVar8;
          pcVar8 = pcVar8 + 4;
          pcVar9 = pcVar9 + 4;
        }
        local_4 = local_4 + 1;
        for (uVar5 = ~uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
          *pcVar9 = *pcVar8;
          pcVar8 = pcVar8 + 1;
          pcVar9 = pcVar9 + 1;
        }
        iVar3 = FUN_00479020(iVar3);
      }
      if (iVar3 == 0) {
        return 1;
      }
      FUN_0048db80(DAT_005561e8);
    }
    FUN_0048db80(DAT_005561d4);
  }
  return 0;
}


// ==== EntityPointer @ 0042ed70 ====

undefined4 EntityPointer(int param_1,int param_2,int *param_3)

{
  if (param_1 == -1) {
    *param_3 = 0;
    return 1;
  }
  if (param_1 == 6) {
    *param_3 = DAT_00556374;
    return 1;
  }
  if (param_1 == 7) {
    *param_3 = (&DAT_0055637c)[param_2];
    return 1;
  }
  if (param_1 == 8) {
    *param_3 = DAT_00556378;
    return 1;
  }
  if (param_1 == 9) {
    *param_3 = DAT_00556388;
    return 1;
  }
  if (param_1 == 0xb) {
    *param_3 = DAT_0055638c;
    return 1;
  }
  if (param_1 == 0xc) {
    *param_3 = DAT_00556390;
    return 1;
  }
  if (param_1 == 0xd) {
    *param_3 = DAT_00556384;
    return 1;
  }
  if (param_1 == 0xf) {
    *param_3 = 0;
    return 1;
  }
  if (param_1 == 5) {
    *param_3 = DAT_00556368;
    return 1;
  }
  if (param_1 == 1) {
    *param_3 = DAT_005561d4 + param_2 * 0x1ec;
    return 1;
  }
  if (param_1 == 2) {
    *param_3 = DAT_005561d8 + param_2 * 0x74;
    return 1;
  }
  if (param_1 == 3) {
    *param_3 = DAT_005561dc + param_2 * 0x74;
    return 1;
  }
  if (param_1 == 4) {
    *param_3 = DAT_005561e0 + param_2 * 0x14c;
    return 1;
  }
  if (param_1 == 0x10) {
    *param_3 = DAT_005563a0;
    return 1;
  }
  if (param_1 == 0x13) {
    *param_3 = DAT_005563b0;
    return 1;
  }
  if (param_1 == 0x11) {
    *param_3 = DAT_005563a4;
    return 1;
  }
  return 0;
}


// ==== MapLoadPipeline @ 0042b510 ====

undefined4
MapLoadPipeline(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
               undefined4 param_5,undefined4 param_6,undefined4 param_7,undefined4 param_8,
               undefined4 param_9,undefined4 param_10,undefined4 param_11,undefined4 param_12,
               undefined4 param_13,undefined4 param_14,undefined4 param_15,undefined4 param_16,
               undefined4 param_17)

{
  undefined2 uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  
  iVar2 = FUN_0044e0c0(DAT_005560d8,param_2,*(undefined4 *)(param_1 + 0x1c),
                       *(undefined4 *)(param_1 + 0x28));
  *(int *)(param_1 + 4) = iVar2;
  uVar3 = 0;
  if (iVar2 != 0) {
    uVar3 = FUN_0045da90(iVar2,&DAT_005563e4,DAT_005563f4);
    *(undefined4 *)(param_1 + 0x84) = uVar3;
    FUN_0044f490(*(undefined4 *)(param_1 + 4),1);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    FUN_0044e6b0(*(undefined4 *)(param_1 + 4),0x70);
    *(undefined4 *)(param_1 + 0x14) = *(undefined4 *)(*(int *)(param_1 + 4) + 8);
    *(undefined4 *)(param_1 + 0x18) = *(undefined4 *)(*(int *)(param_1 + 4) + 0xc);
    FUN_0040ce80(param_1);
    FUN_00466230(param_1);
    if (*(int *)(param_1 + 0x2c) != 0) {
      FUN_00462da0(DAT_005560d8,*(undefined4 *)(param_1 + 0x14),*(undefined4 *)(param_1 + 0x18));
      FUN_004634b0(1);
    }
    iVar2 = FUN_0042bb60(param_1,param_14);
    uVar3 = 0;
    if (iVar2 != 0) {
      puVar4 = (undefined4 *)
               FUN_0048dbd0(*(int *)(param_1 + 0x18) * *(int *)(param_1 + 0x14) * 0x48);
      *(undefined4 **)(param_1 + 0xb8) = puVar4;
      uVar3 = 0;
      if (puVar4 != (undefined4 *)0x0) {
        uVar5 = 0;
        for (iVar2 = (*(int *)(param_1 + 0x18) * *(int *)(param_1 + 0x14) * 9 & 0x1fffffffU) << 1;
            iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = 0;
          puVar4 = puVar4 + 1;
        }
        for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
          *(undefined1 *)puVar4 = 0;
          puVar4 = (undefined4 *)((int)puVar4 + 1);
        }
        if (*(int *)(param_1 + 0x18) != 0) {
          do {
            uVar7 = *(uint *)(param_1 + 0x14);
            uVar6 = 0;
            if (uVar7 != 0) {
              do {
                *(undefined1 *)(*(int *)(param_1 + 0xb8) + (uVar5 * uVar7 + uVar6) * 0x48) = 0;
                *(undefined1 *)
                 (*(int *)(param_1 + 0xb8) + 1 + (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48)
                     = 0;
                *(undefined1 *)
                 (*(int *)(param_1 + 0xb8) + 2 + (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48)
                     = 0;
                *(undefined4 *)
                 (*(int *)(param_1 + 0xb8) + 8 + (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48)
                     = 0;
                *(undefined1 *)
                 (*(int *)(param_1 + 0xb8) + 3 + (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48)
                     = 0;
                *(undefined4 *)
                 (*(int *)(param_1 + 0xb8) + 0x18 +
                 (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48) = 0;
                *(undefined4 *)
                 (*(int *)(param_1 + 0xb8) + 0x14 +
                 (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48) = 0;
                uVar1 = thunk_FUN_0048e170();
                iVar2 = uVar5 * *(int *)(param_1 + 0x14) + uVar6;
                uVar6 = uVar6 + 1;
                *(undefined2 *)(*(int *)(param_1 + 0xb8) + 0x20 + iVar2 * 0x48) = uVar1;
                uVar7 = *(uint *)(param_1 + 0x14);
              } while (uVar6 < uVar7);
            }
            uVar5 = uVar5 + 1;
          } while (uVar5 < *(uint *)(param_1 + 0x18));
        }
        iVar2 = FUN_0042bd20(param_1,param_3,param_4);
        uVar3 = 0;
        if (iVar2 != 0) {
          iVar2 = FUN_0042c480(param_1,param_5,param_6);
          uVar3 = 0;
          if (iVar2 != 0) {
            FUN_0042c6a0(param_1,param_9,param_10);
            FUN_0042bf40(param_1,param_11);
            FUN_0042c760(param_1,param_12,param_13);
            FUN_0042c120(param_1,param_15);
            FUN_0042c060(param_1,param_16);
            FUN_0042c9d0(param_1,param_17);
            iVar2 = FUN_0042cc90(param_1,param_7,param_8);
            if (iVar2 != 0) {
              *(undefined4 *)(param_1 + 0x110) = 1;
            }
            uVar5 = 0;
            if (*(int *)(param_1 + 0x18) != 0) {
              do {
                uVar7 = 0;
                if (*(int *)(param_1 + 0x14) != 0) {
                  do {
                    FUN_0042f6f0(param_1,uVar7,uVar5,1);
                    uVar7 = uVar7 + 1;
                  } while (uVar7 < *(uint *)(param_1 + 0x14));
                }
                uVar5 = uVar5 + 1;
              } while (uVar5 < *(uint *)(param_1 + 0x18));
            }
            FUN_0044e880(*(undefined4 *)(param_1 + 4));
            FUN_0046db90(DAT_005560d8,param_1);
            uVar3 = 1;
          }
        }
      }
    }
  }
  return uVar3;
}


// ==== TerrainCreate @ 0044e0c0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint * TerrainCreate(undefined4 param_1,uint param_2,float param_3,uint param_4)

{
  uint *puVar1;
  int iVar2;
  float *pfVar3;
  undefined1 uVar4;
  int iVar5;
  uint *puVar6;
  uint uVar7;
  uint *puVar8;
  undefined4 uVar9;
  int iVar10;
  uint uVar11;
  uint *puVar12;
  uint uVar13;
  uint uVar14;
  float10 fVar15;
  float10 fVar16;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  uVar11 = 0;
  iVar5 = FUN_004800a0(param_2,0);
  if (iVar5 != 0) {
    puVar6 = (uint *)FUN_0048dbd0(0x73d4);
    if (puVar6 != (uint *)0x0) {
      puVar1 = puVar6 + 2;
      puVar6[4] = (uint)param_3;
      puVar6[5] = param_4;
      FUN_004515a0(iVar5,puVar1,puVar6 + 3);
      uVar7 = puVar6[3] - 1;
      *puVar6 = *puVar1 - 1;
      puVar6[1] = uVar7;
      puVar6[6] = (uint)-((float)(*puVar1 - 1) * param_3 * _DAT_0049e4e4);
      puVar6[0xb] = 0;
      puVar6[7] = (uint)((float)uVar7 * param_3 * _DAT_0049e4e4);
      puVar8 = puVar6 + 0x100d;
      for (iVar10 = 0x20; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar8 = 0;
        puVar8 = puVar8 + 1;
      }
      puVar6[0x1cf4] = 0;
      puVar6[0x12a1] = 0;
      puVar6[0x1c66] = 0;
      puVar6[8] = 0;
      uVar7 = FUN_004894c0(0x3f800000,0x3f800000,0x3f800000,0,0,0,0);
      puVar8 = puVar6 + 0x1c67;
      puVar6[0xc] = uVar7;
      puVar12 = puVar8;
      for (iVar10 = 0x8c; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar12 = 0;
        puVar12 = puVar12 + 1;
      }
      puVar12 = puVar6 + 0x102d;
      for (iVar10 = 0x80; iVar10 != 0; iVar10 = iVar10 + -1) {
        *puVar12 = 0;
        puVar12 = puVar12 + 1;
      }
      iVar10 = 10;
      do {
        *puVar8 = 0xffffffff;
        puVar8 = puVar8 + 0xe;
        iVar10 = iVar10 + -1;
      } while (iVar10 != 0);
      uVar7 = FUN_00473560(param_1,3);
      puVar6[0x1cf3] = uVar7;
      uVar7 = FUN_00473560(param_1,2);
      puVar6[9] = uVar7;
      if (uVar7 != 0) {
        uVar7 = FUN_0048dbd0(*puVar1 * puVar6[3] * 0x28);
        puVar6[10] = uVar7;
        if (uVar7 != 0) {
          local_18 = 0;
          local_14 = 1;
          local_10 = 3;
          local_c = 1;
          local_8 = 2;
          local_4 = 3;
          if (*puVar6 * puVar6[1] != 0) {
            do {
              uVar9 = FUN_004748f0(puVar6[9],4,2,3,&local_18);
              FUN_00474b30(puVar6[9],uVar9,1);
              FUN_00474a60(puVar6[9],uVar9,3);
              uVar11 = uVar11 + 1;
            } while (uVar11 < *puVar6 * puVar6[1]);
          }
          uVar11 = 0;
          if (puVar6[3] != 0) {
            do {
              uVar7 = *puVar1;
              uVar13 = 0;
              if (uVar7 != 0) {
                do {
                  *(undefined1 *)(puVar6[10] + 0x19 + (uVar11 * uVar7 + uVar13) * 0x28) = 0x60;
                  *(undefined1 *)(puVar6[10] + 0x18 + (uVar11 * *puVar1 + uVar13) * 0x28) = 0;
                  uVar7 = *puVar1;
                  uVar14 = puVar6[10];
                  uVar4 = FUN_004515c0(iVar5,uVar13,uVar11,uVar7);
                  *(undefined1 *)(uVar14 + (uVar11 * uVar7 + uVar13) * 0x28 + 0x1a) = uVar4;
                  pfVar3 = (float *)(puVar6[10] + 0x1c + (uVar11 * *puVar1 + uVar13) * 0x28);
                  fVar15 = (float10)FUN_004798b0(0,0x3f800000);
                  *pfVar3 = (float)fVar15;
                  fVar15 = (float10)FUN_004798b0(0,0x3f800000);
                  fVar16 = (float10)_DAT_0049e4e8 /
                           SQRT((float10)*pfVar3 * (float10)*pfVar3 + fVar15 * fVar15);
                  *pfVar3 = (float)(fVar16 * (float10)*pfVar3);
                  pfVar3[1] = (float)(fVar16 * fVar15);
                  iVar10 = uVar11 * *puVar1 + uVar13;
                  pfVar3 = (float *)(puVar6[10] + 0x1c + iVar10 * 0x28);
                  *pfVar3 = *(float *)(puVar6[10] + 0x1c + iVar10 * 0x28) * _DAT_0049e4ec;
                  pfVar3[1] = pfVar3[1] * _DAT_0049e4ec;
                  iVar10 = uVar11 * *puVar1 + uVar13;
                  uVar13 = uVar13 + 1;
                  *(undefined1 *)(puVar6[10] + 0x24 + iVar10 * 0x28) = 0;
                  uVar7 = *puVar1;
                } while (uVar13 < uVar7);
              }
              uVar11 = uVar11 + 1;
            } while (uVar11 < puVar6[3]);
          }
          FUN_0044e4d0(puVar6);
          param_2 = 0;
          if (puVar6[1] != 0) {
            do {
              param_3 = 0.0;
              if (*puVar6 != 0) {
                do {
                  iVar10 = puVar6[10] + (param_2 * *puVar1 + (int)param_3) * 0x28;
                  iVar2 = puVar6[10] + ((param_2 + 1) * *puVar1 + (int)param_3) * 0x28;
                  uVar11 = (uint)*(byte *)(iVar10 + 0x1a) - (uint)*(byte *)(iVar2 + 0x42);
                  uVar14 = (uint)*(byte *)(iVar10 + 0x42) - (uint)*(byte *)(iVar2 + 0x1a);
                  uVar7 = (int)uVar11 >> 0x1f;
                  uVar13 = (int)uVar14 >> 0x1f;
                  if ((uVar11 ^ uVar7) - uVar7 < (uVar14 ^ uVar13) - uVar13) {
                    FUN_0044edf0(puVar6,param_3,param_2,1);
                  }
                  else {
                    FUN_004514f0(puVar6,param_3,param_2);
                  }
                  param_3 = (float)((int)param_3 + 1);
                } while ((uint)param_3 < *puVar6);
              }
              param_2 = param_2 + 1;
            } while (param_2 < puVar6[1]);
          }
          FUN_0048db80(iVar5);
          return puVar6;
        }
        FUN_00472a40(puVar6[9]);
      }
      FUN_0048db80(puVar6);
    }
    FUN_0048db80(iVar5);
  }
  return (uint *)0x0;
}


// ==== MapFileReadHeader @ 004515a0 ====

void MapFileReadHeader(int param_1,undefined4 *param_2,undefined4 *param_3)

{
  *param_2 = *(undefined4 *)(param_1 + 8);
  *param_3 = *(undefined4 *)(param_1 + 0xc);
  return;
}


// ==== MapFileGetCell @ 004515c0 ====

undefined4 MapFileGetCell(int param_1,int param_2,int param_3,int param_4)

{
  param_2 = param_4 * param_3 + param_2;
  return CONCAT22((short)((uint)param_2 >> 0x10),*(undefined2 *)(param_1 + 0x10 + param_2 * 2));
}


// ==== LoadPredugLayer @ 0042bd20 ====

undefined4 LoadPredugLayer(int param_1,int param_2,int param_3)

{
  int *piVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  int local_58;
  int local_54;
  uint local_50;
  uint local_4c;
  int local_48 [18];
  
  if ((param_2 != 0) && (iVar3 = FUN_00480170(param_2,0), iVar3 != -1)) {
    local_54 = iVar3;
    FUN_004334c0(iVar3,&param_2,&local_58);
    if ((param_2 == *(int *)(param_1 + 0x14)) && (local_58 == *(int *)(param_1 + 0x18))) {
      uVar7 = 1;
      iVar4 = param_2;
      iVar6 = local_58;
      if (local_58 != 1 && local_58 != 2) {
        do {
          uVar8 = 1;
          if (iVar4 != 1 && iVar4 != 2) {
            do {
              iVar3 = FUN_004319e0(local_54,uVar8,uVar7);
              iVar3 = iVar3 - param_3;
              if ((iVar3 == 1) || (iVar3 == 3)) {
                FUN_00430530(param_1,uVar8,uVar7,1);
                local_48[0] = -1;
                local_48[1] = 0xffffffff;
                local_48[3] = 0xffffffff;
                local_48[5] = 0xffffffff;
                local_48[0xc] = 0xffffffff;
                local_48[0xe] = 0xffffffff;
                local_48[0x10] = 0;
                local_48[2] = 0;
                local_48[4] = 1;
                local_48[6] = 1;
                local_48[7] = 0;
                local_48[8] = 1;
                local_48[9] = 1;
                local_48[10] = 0;
                local_48[0xb] = 1;
                local_48[0xd] = 1;
                local_48[0xf] = 0;
                local_48[0x11] = 0;
                piVar5 = local_48;
                iVar4 = 9;
                do {
                  piVar1 = piVar5 + 1;
                  iVar6 = *piVar5;
                  piVar5 = piVar5 + 2;
                  puVar2 = (uint *)(*(int *)(param_1 + 0xb8) + 8 +
                                   ((*piVar1 + uVar7) * *(int *)(param_1 + 0x14) + uVar8 + iVar6) *
                                   0x48);
                  iVar4 = iVar4 + -1;
                  *puVar2 = *puVar2 | 0x4000000;
                } while (iVar4 != 0);
              }
              else if ((iVar3 == 2) || (iVar3 == 4)) {
                iVar4 = uVar7 * *(int *)(param_1 + 0x14) + uVar8;
                *(uint *)(*(int *)(param_1 + 0xb8) + 8 + iVar4 * 0x48) =
                     *(uint *)(*(int *)(param_1 + 0xb8) + 8 + iVar4 * 0x48) | 0x20000;
              }
              if (iVar3 == 3) {
                iVar4 = uVar7 * *(int *)(param_1 + 0x14) + uVar8;
                *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar4 * 0x48) =
                     *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar4 * 0x48) | 0x20;
                local_50 = uVar8;
                local_4c = uVar7;
                FUN_0044a4b0(&local_50);
              }
              if (iVar3 == 4) {
                iVar3 = uVar7 * *(int *)(param_1 + 0x14) + uVar8;
                *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar3 * 0x48) =
                     *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar3 * 0x48) | 0x200;
              }
              uVar8 = uVar8 + 1;
              iVar4 = param_2;
              iVar6 = local_58;
            } while (uVar8 < param_2 - 1U);
          }
          uVar7 = uVar7 + 1;
          iVar3 = local_54;
        } while (uVar7 < iVar6 - 1U);
      }
      FUN_00489460(iVar3);
      return 1;
    }
    FUN_00489460(iVar3);
  }
  return 0;
}


// ==== LoadErodeCryoreLayers @ 0042c480 ====

undefined4 LoadErodeCryoreLayers(int param_1,uint param_2,int param_3)

{
  char cVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint local_c;
  uint local_8;
  uint local_4;
  
  iVar2 = FUN_00480170(param_2,0);
  if (iVar2 != -1) {
    FUN_004334c0(iVar2,&param_2,&local_c);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_c == *(uint *)(param_1 + 0x18))) {
      uVar6 = 0;
      uVar4 = param_2;
      uVar5 = local_c;
      if (local_c != 0) {
        do {
          uVar7 = 0;
          if (uVar4 != 0) {
            do {
              iVar3 = FUN_004319e0(iVar2,uVar7,uVar6);
              iVar3 = iVar3 - param_3;
              if (iVar3 == 5) {
                iVar3 = 4;
              }
              *(char *)(*(int *)(param_1 + 0xb8) + 2 +
                       (uVar6 * *(int *)(param_1 + 0x14) + uVar7) * 0x48) = (char)iVar3;
              iVar3 = *(int *)(param_1 + 0xb8) + (uVar6 * *(int *)(param_1 + 0x14) + uVar7) * 0x48;
              cVar1 = *(char *)(iVar3 + 2);
              if (cVar1 == '\x06') {
                *(undefined1 *)(iVar3 + 7) = 4;
              }
              else if (cVar1 == '\v') {
                local_8 = uVar7;
                local_4 = uVar6;
                FUN_0044a390(&local_8);
                FUN_004311d0(param_1,&local_8,1);
              }
              uVar7 = uVar7 + 1;
              uVar4 = param_2;
              uVar5 = local_c;
            } while (uVar7 < param_2);
          }
          uVar6 = uVar6 + 1;
        } while (uVar6 < uVar5);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== LoadEmergeLayer @ 0042bf40 ====

undefined4 LoadEmergeLayer(int param_1,uint param_2)

{
  bool bVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint local_10;
  int local_c;
  uint local_8;
  uint local_4;
  
  if ((param_2 != 0) && (iVar3 = FUN_00480170(param_2,0), iVar3 != -1)) {
    local_c = iVar3;
    FUN_004334c0(iVar3,&param_2,&local_10);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_10 == *(uint *)(param_1 + 0x18))) {
      uVar7 = 0;
      uVar4 = param_2;
      uVar5 = local_10;
      if (local_10 != 0) {
        do {
          uVar6 = 0;
          if (uVar4 != 0) {
            do {
              local_8 = uVar6;
              local_4 = uVar7;
              uVar4 = FUN_004319e0(local_c,uVar6,uVar7);
              bVar1 = false;
              if ((uVar4 == 0) || ((uVar4 & 1) != 0)) {
                uVar4 = uVar4 + 1;
              }
              else {
                bVar1 = true;
              }
              *(char *)(*(int *)(param_1 + 0xb8) + 6 +
                       (uVar7 * *(int *)(param_1 + 0x14) + uVar6) * 0x48) = (char)(uVar4 >> 1);
              if (bVar1) {
                sVar2 = thunk_FUN_0048e170();
                uVar4 = (int)sVar2 >> 0x1f;
                FUN_0040ea10(&local_8,(((int)sVar2 ^ uVar4) - uVar4 & 3 ^ uVar4) - uVar4);
              }
              uVar6 = uVar6 + 1;
              uVar4 = param_2;
              uVar5 = local_10;
            } while (uVar6 < param_2);
          }
          uVar7 = uVar7 + 1;
          iVar3 = local_c;
        } while (uVar7 < uVar5);
      }
      FUN_00489460(iVar3);
      return 1;
    }
    FUN_00489460(iVar3);
  }
  return 0;
}


// ==== LoadPathAiLayers @ 0042c6a0 ====

undefined4 LoadPathAiLayers(int param_1,uint param_2,char param_3)

{
  char cVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint local_4;
  
  iVar2 = FUN_00480170(param_2,0);
  if (iVar2 != -1) {
    FUN_004334c0(iVar2,&param_2,&local_4);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_4 == *(uint *)(param_1 + 0x18))) {
      uVar7 = 0;
      uVar4 = param_2;
      uVar5 = local_4;
      if (local_4 != 0) {
        do {
          uVar6 = 0;
          if (uVar4 != 0) {
            do {
              cVar1 = FUN_004319e0(iVar2,uVar6,uVar7);
              iVar3 = uVar7 * *(int *)(param_1 + 0x14) + uVar6;
              uVar6 = uVar6 + 1;
              *(char *)(*(int *)(param_1 + 0xb8) + 5 + iVar3 * 0x48) = cVar1 - param_3;
              uVar4 = param_2;
              uVar5 = local_4;
            } while (uVar6 < param_2);
          }
          uVar7 = uVar7 + 1;
        } while (uVar7 < uVar5);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== LoadFallinLayer @ 0042c760 ====

undefined4 LoadFallinLayer(int param_1,uint param_2,int param_3)

{
  uint *puVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint local_10;
  int local_c;
  uint local_8;
  uint local_4;
  
  if ((param_2 != 0) && (iVar2 = FUN_00480170(param_2,0), iVar2 != -1)) {
    local_c = iVar2;
    FUN_004334c0(iVar2,&param_2,&local_10);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_10 == *(uint *)(param_1 + 0x18))) {
      uVar5 = 0;
      uVar3 = param_2;
      uVar4 = local_10;
      if (local_10 != 0) {
        do {
          uVar6 = 0;
          if (uVar3 != 0) {
            do {
              local_8 = uVar6;
              local_4 = uVar5;
              iVar2 = FUN_004319e0(local_c,uVar6,uVar5);
              puVar1 = (uint *)(*(int *)(param_1 + 0xb8) + 8 +
                               (uVar5 * *(int *)(param_1 + 0x14) + uVar6) * 0x48);
              uVar3 = *puVar1;
              if ((uVar3 & 8) != 0) {
                if (iVar2 - param_3 == 2) {
                  *puVar1 = uVar3 | 0x20000000;
                }
                else if ((((iVar2 - param_3 == 1) &&
                          (iVar2 = *(int *)(DAT_005560d4 + 0xb8) +
                                   (*(int *)(DAT_005560d4 + 0x14) * local_4 + local_8) * 0x48,
                          (*(uint *)(iVar2 + 8) & 0x400) == 0)) &&
                         ((*(byte *)(iVar2 + 0xc) & 4) == 0)) &&
                        (((*(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 +
                                    (*(int *)(DAT_005560d4 + 0x14) * local_4 + local_8) * 0x48) &
                          0x8000) == 0 && (iVar2 = FUN_004090a0(&local_8), iVar2 == 0)))) {
                  iVar2 = *(int *)(DAT_005560d4 + 0x14) * local_4 + local_8;
                  *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) =
                       *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) & 0xffdfffff;
                  iVar2 = *(int *)(DAT_005560d4 + 0x14) * local_4 + local_8;
                  *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) =
                       *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) & 0xdfffffff;
                  iVar2 = *(int *)(DAT_005560d4 + 0x14) * local_4 + local_8;
                  *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) =
                       *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) & 0xbfffffff;
                  FUN_00439990();
                  FUN_00402a40(&local_8);
                  FUN_00484c90(&local_8);
                  iVar2 = 4;
                  do {
                    FUN_004030b0(&local_8,0x3d);
                    iVar2 = iVar2 + -1;
                  } while (iVar2 != 0);
                  iVar2 = *(int *)(DAT_005560d4 + 0x14) * local_4 + local_8;
                  *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) =
                       *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar2 * 0x48) | 3;
                }
              }
              uVar6 = uVar6 + 1;
              uVar3 = param_2;
              uVar4 = local_10;
            } while (uVar6 < param_2);
          }
          uVar5 = uVar5 + 1;
          iVar2 = local_c;
        } while (uVar5 < uVar4);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== LoadBlockPointers @ 0042c9d0 ====

undefined4 LoadBlockPointers(int param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint local_10;
  int local_c;
  uint local_8;
  undefined4 uStack_4;
  
  DAT_00556f18 = 0;
  if ((param_2 != 0) && (iVar1 = FUN_00480170(param_2,0), iVar1 != -1)) {
    local_c = iVar1;
    FUN_004334c0(iVar1,&param_2,&local_10);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_10 == *(uint *)(param_1 + 0x18))) {
      uVar4 = 0;
      uVar2 = param_2;
      uVar3 = local_10;
      if (local_10 != 0) {
        do {
          uVar5 = 0;
          if (uVar2 != 0) {
            do {
              uVar2 = FUN_004319e0(local_c,uVar5,uVar4);
              *(undefined4 *)
               (*(int *)(param_1 + 0xb8) + 0x3c + (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48)
                   = 0;
              if (uVar2 != 0) {
                *(uint *)(*(int *)(param_1 + 0xb8) + 0x3c +
                         (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48) = uVar2;
                iVar1 = *(int *)(param_1 + 0xb8) + (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48
                ;
                local_8 = (uint)(int)*(short *)(iVar1 + 0x20) % (uVar2 * DAT_00556f14 * 0x19);
                uStack_4 = 0;
                *(float *)(iVar1 + 0x40) = (float)local_8;
                *(undefined4 *)
                 (*(int *)(param_1 + 0xb8) + 0x38 +
                 (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48) = 0;
                if (4 < uVar2) {
                  *(uint *)(*(int *)(param_1 + 0xb8) + 0x3c +
                           (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48) = uVar2 - 4;
                  *(undefined4 *)
                   (*(int *)(param_1 + 0xb8) + 0x38 +
                   (uVar4 * *(int *)(param_1 + 0x14) + uVar5) * 0x48) = 1;
                }
              }
              uVar5 = uVar5 + 1;
              uVar2 = param_2;
              uVar3 = local_10;
            } while (uVar5 < param_2);
          }
          uVar4 = uVar4 + 1;
          iVar1 = local_c;
        } while (uVar4 < uVar3);
      }
      DAT_00556f18 = 1;
      FUN_00489460(iVar1);
      return 1;
    }
    FUN_00489460(iVar1);
  }
  return 0;
}


// ==== LoadTerrainMap @ 0042cc90 ====

undefined4 LoadTerrainMap(int param_1,uint param_2,undefined4 param_3)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint local_4;
  
  if ((param_2 != 0) && (iVar2 = FUN_00480170(param_2,0), iVar2 != -1)) {
    FUN_004334c0(iVar2,&param_2,&local_4);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_4 == *(uint *)(param_1 + 0x18))) {
      uVar7 = 0;
      uVar3 = param_2;
      uVar5 = local_4;
      if (local_4 != 0) {
        do {
          uVar6 = 0;
          if (uVar3 != 0) {
            do {
              cVar1 = FUN_004319e0(iVar2,uVar6,uVar7);
              iVar4 = uVar7 * *(int *)(param_1 + 0x14) + uVar6;
              uVar6 = uVar6 + 1;
              *(char *)(*(int *)(param_1 + 0xb8) + 4 + iVar4 * 0x48) = cVar1 - (char)param_3;
              uVar3 = param_2;
              uVar5 = local_4;
            } while (uVar6 < param_2);
          }
          uVar7 = uVar7 + 1;
        } while (uVar7 < uVar5);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== LoadLayerL @ 0042c120 ====

undefined4 LoadLayerL(int param_1,uint param_2)

{
  uint *puVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  undefined4 *puVar8;
  uint local_c;
  uint local_8;
  int local_4;
  
  if (param_2 != 0) {
    iVar2 = FUN_00480170(param_2,0);
    local_4 = iVar2;
    if (((iVar2 != -1) &&
        (FUN_004334c0(iVar2,&local_c,&local_8), local_c == *(uint *)(param_1 + 0x14))) &&
       (local_8 == *(uint *)(param_1 + 0x18))) {
      param_2 = 0;
      uVar4 = local_c;
      uVar5 = local_8;
      if (local_8 != 0) {
        do {
          uVar7 = 0;
          if (uVar4 != 0) {
            do {
              iVar2 = FUN_004319e0(local_4,uVar7,param_2);
              if (iVar2 != 0) {
                uVar5 = (iVar2 - 1U) % 6;
                uVar6 = (iVar2 - 1U) / 6;
                uVar4 = uVar6 + 1;
                if (*(uint *)(param_1 + 0xc) < uVar4) {
                  if (*(int *)(param_1 + 8) == 0) {
                    uVar3 = FUN_0048dbd0(uVar4 * 0x4c);
                  }
                  else {
                    uVar3 = FUN_0048dc80(*(int *)(param_1 + 8),uVar4 * 0x4c);
                  }
                  *(undefined4 *)(param_1 + 8) = uVar3;
                  puVar8 = (undefined4 *)(*(int *)(param_1 + 8) + *(int *)(param_1 + 0xc) * 0x4c);
                  for (uVar4 = ((uVar6 - *(int *)(param_1 + 0xc)) + 1) * 0x13 & 0x3fffffff;
                      uVar4 != 0; uVar4 = uVar4 - 1) {
                    *puVar8 = 0;
                    puVar8 = puVar8 + 1;
                  }
                  for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
                    *(undefined1 *)puVar8 = 0;
                    puVar8 = (undefined4 *)((int)puVar8 + 1);
                  }
                  *(uint *)(param_1 + 0xc) = uVar6 + 1;
                }
                if (uVar5 == 0) {
                  iVar2 = param_2 * *(int *)(param_1 + 0x14) + uVar7;
                  *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar2 * 0x48) =
                       *(uint *)(*(int *)(param_1 + 0xb8) + 0xc + iVar2 * 0x48) | 0x80;
                  *(uint *)(uVar6 * 0x4c + 4 + *(int *)(param_1 + 8)) = uVar7;
                  *(uint *)(uVar6 * 0x4c + 8 + *(int *)(param_1 + 8)) = param_2;
                }
                else {
                  puVar1 = (uint *)(*(int *)(param_1 + 0xb8) + 0xc +
                                   (param_2 * *(int *)(param_1 + 0x14) + uVar7) * 0x48);
                  *puVar1 = *puVar1 | 0x40;
                  iVar2 = uVar6 * 0x4c;
                  *(uint *)(iVar2 + uVar5 * 0xc + 4 + *(int *)(param_1 + 8)) = uVar7;
                  *(uint *)(uVar5 * 0xc + iVar2 + 8 + *(int *)(param_1 + 8)) = param_2;
                  *(undefined4 *)(iVar2 + (uVar5 + 1) * 0xc + *(int *)(param_1 + 8)) = 1;
                }
              }
              uVar7 = uVar7 + 1;
              uVar4 = local_c;
              uVar5 = local_8;
            } while (uVar7 < local_c);
          }
          param_2 = param_2 + 1;
          iVar2 = local_4;
        } while (param_2 < uVar5);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== LoadLayerM @ 0042c060 ====

undefined4 LoadLayerM(int param_1,uint param_2)

{
  undefined1 uVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint local_4;
  
  if ((param_2 != 0) && (iVar2 = FUN_00480170(param_2,0), iVar2 != -1)) {
    FUN_004334c0(iVar2,&param_2,&local_4);
    if ((param_2 == *(uint *)(param_1 + 0x14)) && (local_4 == *(uint *)(param_1 + 0x18))) {
      uVar7 = 0;
      uVar3 = param_2;
      uVar5 = local_4;
      if (local_4 != 0) {
        do {
          uVar6 = 0;
          if (uVar3 != 0) {
            do {
              uVar1 = FUN_004319e0(iVar2,uVar6,uVar7);
              iVar4 = uVar7 * *(int *)(param_1 + 0x14) + uVar6;
              uVar6 = uVar6 + 1;
              *(undefined1 *)(*(int *)(param_1 + 0xb8) + 0x34 + iVar4 * 0x48) = uVar1;
              uVar3 = param_2;
              uVar5 = local_4;
            } while (uVar6 < param_2);
          }
          uVar7 = uVar7 + 1;
        } while (uVar7 < uVar5);
      }
      FUN_00489460(iVar2);
      return 1;
    }
    FUN_00489460(iVar2);
  }
  return 0;
}


// ==== CellInitPass @ 0042f6f0 ====

/* WARNING: Removing unreachable block (ram,0x0042fed8) */
/* WARNING: Removing unreachable block (ram,0x0042fee3) */
/* WARNING: Removing unreachable block (ram,0x0042ff72) */
/* WARNING: Removing unreachable block (ram,0x0042ff7d) */
/* WARNING: Recovered jumptable eliminated as dead code */

void CellInitPass(int param_1,float param_2,uint param_3)

{
  int iVar1;
  undefined1 uVar2;
  byte bVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined4 uVar8;
  uint uVar9;
  uint *puVar10;
  ulonglong uVar11;
  undefined1 local_60 [4];
  int local_5c;
  float local_58;
  float local_54;
  float *local_50;
  undefined4 local_4c;
  float local_48;
  uint local_44;
  undefined4 local_40;
  int local_3c;
  float local_38;
  uint local_34;
  uint local_30 [12];
  
  if (*(int *)(param_1 + 0x14) - 1U <= (uint)param_2) {
    return;
  }
  if (*(int *)(param_1 + 0x18) - 1U <= param_3) {
    return;
  }
  local_4c = *(undefined4 *)(param_1 + 4);
  iVar4 = param_3 * *(int *)(param_1 + 0x14) + (int)param_2;
  local_5c = 0;
  local_30[0] = 0;
  uVar5 = *(uint *)(*(int *)(param_1 + 0xb8) + 8 + iVar4 * 0x48) & 0x20000000;
  local_30[1] = 0;
  iVar4 = *(int *)(param_1 + 0xb8) + iVar4 * 0x48;
  local_30[2] = 1;
  local_30[3] = 0;
  local_30[4] = 1;
  local_30[5] = 1;
  local_30[6] = 0;
  local_30[7] = 1;
  local_3c = 1;
  if ((uVar5 == 0) || ((*(uint *)(iVar4 + 0xc) & 0x400) != 0)) {
    if ((uVar5 == 0) && ((*(uint *)(iVar4 + 0xc) & 0x400) != 0)) {
      FUN_004374d0(0);
      uVar5 = *(uint *)(iVar4 + 0xc) & 0xfffffbff;
      goto LAB_0042f7a5;
    }
  }
  else {
    FUN_004374d0(1);
    uVar5 = *(uint *)(iVar4 + 0xc) | 0x400;
LAB_0042f7a5:
    *(uint *)(iVar4 + 0xc) = uVar5;
  }
  iVar7 = *(int *)(param_1 + 0x14);
  iVar1 = *(int *)(param_1 + 0xb8);
  local_50 = &local_38;
  uVar5 = 0;
  puVar10 = local_30;
  do {
    if (*(char *)(iVar1 + ((puVar10[1] + param_3) * iVar7 + (int)param_2 + *puVar10) * 0x48) ==
        '\x01') {
      local_48 = (float)CONCAT31(local_48._1_3_,(char)uVar5);
      local_5c = local_5c + 1;
    }
    else {
      *(char *)local_50 = (char)uVar5;
      local_50 = (float *)((int)local_50 + 1);
    }
    uVar5 = uVar5 + 1;
    puVar10 = puVar10 + 2;
  } while (uVar5 < 4);
  uVar5 = *(uint *)(iVar4 + 8);
  uVar6 = uVar5 & 0xffffdf27;
  *(uint *)(iVar4 + 8) = uVar6;
  if (local_5c == 4) {
    local_38 = param_2;
    local_34 = param_3;
    *(uint *)(iVar4 + 8) = uVar5 & 0xffffdf07 | 0xc;
    *(undefined1 *)(iVar4 + 3) = 0;
    if ((uVar5 & 8) == 0) {
      local_48 = param_2;
      local_44 = param_3;
      FUN_00402ae0(&local_38,0);
      FUN_00431450(param_1,&local_48,1);
      if ((DAT_00556ecc & 0x800000) != 0) {
        FUN_00452060(0xe,0,*(undefined1 *)
                            (*(int *)(DAT_005560d4 + 0xb8) + 3 +
                            (*(int *)(DAT_005560d4 + 0x14) * param_3 + (int)param_2) * 0x48),
                     &local_48);
      }
    }
    if ((*(uint *)(iVar4 + 8) & 0x400000) == 0) {
      iVar7 = *(int *)(DAT_005560d4 + 0xb8) +
              (*(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38) * 0x48;
      if (((((*(uint *)(iVar7 + 8) & 0x400) == 0) && ((*(byte *)(iVar7 + 0xc) & 4) == 0)) &&
          ((*(byte *)(*(int *)(DAT_005560d4 + 0xb8) + 9 +
                     (*(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38) * 0x48) & 0x80) == 0
          )) && (iVar7 = FUN_004090a0(&local_38), iVar7 == 0)) {
        iVar7 = *(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38;
        *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) =
             *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) & 0xffdfffff;
        iVar7 = *(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38;
        *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) =
             *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) & 0xdfffffff;
        iVar7 = *(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38;
        *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) =
             *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) & 0xbfffffff;
        FUN_00439990();
        FUN_00402a40(&local_38);
        FUN_00484c90(&local_38);
        iVar7 = 4;
        do {
          FUN_004030b0(&local_38,0x28);
          iVar7 = iVar7 + -1;
        } while (iVar7 != 0);
        iVar7 = *(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38;
        *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) =
             *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) | 3;
      }
      iVar7 = *(int *)(DAT_005560d4 + 0x14) * local_34 + (int)local_38;
      *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) =
           *(uint *)(*(int *)(DAT_005560d4 + 0xb8) + 8 + iVar7 * 0x48) | 0x400000;
      FUN_0041cf80(&local_38);
    }
    iVar7 = FUN_0040e1e0(param_1,param_2,param_3);
    FUN_0040e350(param_1,param_2,param_3,iVar7 != 0);
    if ((*(char *)(iVar4 + 2) == '\x06') ||
       (uVar5 = *(uint *)(iVar4 + 8), (uVar5 & 0x80000000) != 0)) {
      FUN_0040ee50(&local_38,1);
      if ((*(uint *)(iVar4 + 8) & 0x200) == 0) {
        if (*(char *)(iVar4 + 7) == '\x04') {
          *(undefined1 *)(iVar4 + 1) = 0x46;
          FUN_00450960(*(undefined4 *)(param_1 + 4),param_2,param_3,1);
          if ((*(uint *)(iVar4 + 8) & 0x4000) == 0) {
            if ((int)*(short *)(iVar4 + 0x20) % 3 == 0) {
              local_48 = 0.1;
              local_44 = 0x3dcccccd;
              local_40 = 0xbf800000;
              uVar8 = FUN_00465430(0,param_2,param_3,&local_48,0x3f19999a,0x3ecccccd,0,0x3f800000,
                                   0x3e99999a,0,(int)*(short *)(iVar4 + 0x20),0x26);
              *(undefined4 *)(iVar4 + 0x1c) = uVar8;
              *(uint *)(iVar4 + 8) = *(uint *)(iVar4 + 8) | 0x4000;
            }
          }
          else {
            FUN_00465ac0(*(undefined4 *)(iVar4 + 0x1c),0);
          }
        }
        else {
          FUN_00450960(*(undefined4 *)(param_1 + 4),param_2,param_3,0);
          switch(*(undefined1 *)(iVar4 + 7)) {
          case 0:
            *(undefined1 *)(iVar4 + 1) = 6;
            break;
          case 1:
            *(undefined1 *)(iVar4 + 1) = 0x16;
            break;
          case 2:
            *(undefined1 *)(iVar4 + 1) = 0x26;
            break;
          case 3:
            *(undefined1 *)(iVar4 + 1) = 0x36;
          }
        }
      }
      else {
        FUN_00450960(*(undefined4 *)(param_1 + 4),param_2,param_3,0);
        *(undefined1 *)(iVar4 + 1) = 0x47;
        if ((*(uint *)(iVar4 + 8) & 0x4000) != 0) {
          FUN_00465ac0(*(undefined4 *)(iVar4 + 0x1c),1);
        }
      }
    }
    else if (*(char *)(iVar4 + 2) == '\t') {
      *(undefined1 *)(iVar4 + 1) = 0x45;
      FUN_00450960(*(undefined4 *)(param_1 + 4),param_2,param_3,1);
    }
    else if ((uVar5 & 0x100000) == 0) {
      if ((uVar5 & 0x20000000) == 0) {
        if ((*(byte *)(iVar4 + 0xc) & 0x20) == 0) {
          if ((uVar5 & 0x200000) == 0) {
            switch(uVar5 & 3) {
            case 0:
              *(undefined1 *)(iVar4 + 1) = 0x13;
              break;
            case 1:
              *(undefined1 *)(iVar4 + 1) = 0x12;
              break;
            case 2:
              *(undefined1 *)(iVar4 + 1) = 0x11;
              break;
            case 3:
              *(undefined1 *)(iVar4 + 1) = 0x10;
            }
          }
          else if ((uVar5 & 0x40000000) == 0) {
            *(undefined1 *)(iVar4 + 1) = 0;
          }
          else {
            *(undefined1 *)(iVar4 + 1) = 0x61;
          }
        }
        else {
          *(undefined1 *)(iVar4 + 1) = 0x30;
        }
      }
      else {
        uVar2 = FUN_0042f350(param_2,param_3,iVar4 + 3,*(uint *)(iVar4 + 0xc) & 0x100);
        *(undefined1 *)(iVar4 + 1) = uVar2;
      }
    }
    else if ((*(uint *)(iVar4 + 0xc) & 0x100) == 0) {
      *(undefined1 *)(iVar4 + 1) = 0x76;
    }
    else {
      *(undefined1 *)(iVar4 + 1) = 0x66;
    }
    goto switchD_0042fb0e_default;
  }
  if (local_5c != 2) {
    if ((local_5c != 1) && (local_5c != 3)) {
      if ((uVar5 & 4) != 0) {
        switch(*(undefined1 *)(iVar4 + 2)) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          break;
        default:
switchD_0042fea8_default:
          *(undefined1 *)(iVar4 + 3) = 0;
          goto switchD_0042fb0e_default;
        }
      }
      *(undefined1 *)(iVar4 + 1) = 0x70;
      goto switchD_0042fea8_default;
    }
    *(uint *)(iVar4 + 8) = uVar5 & 0xffffdf07 | 0x14;
    if (local_5c == 1) {
      switch(*(undefined1 *)(iVar4 + 2)) {
      case 1:
      case 7:
        *(undefined1 *)(iVar4 + 1) = 0x35;
        break;
      case 2:
        *(undefined1 *)(iVar4 + 1) = 0x34;
        break;
      case 3:
        *(undefined1 *)(iVar4 + 1) = 0x33;
        break;
      case 4:
        *(undefined1 *)(iVar4 + 1) = 0x32;
        break;
      case 5:
        *(undefined1 *)(iVar4 + 1) = 0x31;
      }
      *(uint *)(iVar4 + 8) = uVar5 & 0xffffdf07 | 0x54;
      local_34 = param_3;
      *(byte *)(iVar4 + 3) = SUB41(local_48,0) + 2U & 3;
      local_38 = param_2;
      FUN_00431450(param_1,&local_38,1);
    }
    else if (local_5c == 3) {
      switch(*(undefined1 *)(iVar4 + 2)) {
      case 1:
      case 7:
        *(undefined1 *)(iVar4 + 1) = 0x55;
        break;
      case 2:
        *(undefined1 *)(iVar4 + 1) = 0x54;
        break;
      case 3:
        *(undefined1 *)(iVar4 + 1) = 0x53;
        break;
      case 4:
        *(undefined1 *)(iVar4 + 1) = 0x52;
        break;
      case 5:
        *(undefined1 *)(iVar4 + 1) = 0x51;
      }
      local_48 = param_2;
      local_44 = param_3;
      FUN_00431450(param_1,&local_48,1);
      FUN_00402b30(&local_48);
      *(undefined1 *)(iVar4 + 3) = local_38._0_1_;
      *(uint *)(iVar4 + 8) = *(uint *)(iVar4 + 8) | 0x80;
    }
    uVar8 = local_4c;
    FUN_004507d0(local_4c,param_2,param_3,local_60,local_60,&local_58);
    FUN_004507d0(uVar8,(int)param_2 + 1U,param_3 + 1,local_60,local_60,&local_54);
    local_48 = local_58 - local_54;
    FUN_004507d0(uVar8,(int)param_2 + 1U,param_3,local_60,local_60,&local_58);
    FUN_004507d0(uVar8,param_2,param_3 + 1,local_60,local_60,&local_54);
    FUN_0044edf0(uVar8,param_2,param_3,ABS(local_58 - local_54) < ABS(local_48));
    local_38 = param_2;
    local_34 = param_3;
    FUN_00431450(param_1,&local_38,1);
    goto switchD_0042fb0e_default;
  }
  local_30[1] = 1;
  local_30[2] = 1;
  local_30[10] = 1;
  local_30[0] = 0;
  local_30[6] = 0;
  local_30[8] = 0;
  local_30[5] = 3;
  local_30[7] = 3;
  local_30[0xb] = 3;
  uVar9 = 0;
  local_30[3] = 2;
  local_30[4] = 2;
  local_30[9] = 2;
  *(uint *)(iVar4 + 8) = uVar6 | 0x14;
  puVar10 = local_30 + 1;
  do {
    if ((puVar10[-1] == ((uint)local_38 & 0xff)) && (*puVar10 == ((uint)local_38 >> 8 & 0xff))) {
      bVar3 = (byte)uVar9;
      goto LAB_0042fd13;
    }
    uVar9 = uVar9 + 1;
    puVar10 = puVar10 + 2;
  } while (uVar9 < 6);
  bVar3 = param_2._0_1_;
LAB_0042fd13:
  if (3 < bVar3) {
    switch(*(undefined1 *)(iVar4 + 2)) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      *(undefined1 *)(iVar4 + 1) = 0x77;
    }
    *(uint *)(iVar4 + 8) =
         CONCAT22((short)(uVar6 >> 0x10),CONCAT11((char)(uVar6 >> 8),(char)(uVar6 | 0x14))) | 0x2000
    ;
    *(byte *)(iVar4 + 3) = bVar3 - 4;
    FUN_0044edf0(local_4c,param_2,param_3,bVar3 - 4 & 1);
    goto switchD_0042fb0e_default;
  }
  if ((uVar5 & 0x20) != 0) {
    switch(*(undefined1 *)(iVar4 + 2)) {
    case 1:
    case 7:
      *(undefined1 *)(iVar4 + 1) = 0x25;
      *(byte *)(iVar4 + 3) = bVar3;
      break;
    case 2:
      *(undefined1 *)(iVar4 + 1) = 0x24;
      *(byte *)(iVar4 + 3) = bVar3;
      break;
    case 3:
      *(undefined1 *)(iVar4 + 1) = 0x23;
      *(byte *)(iVar4 + 3) = bVar3;
      break;
    case 4:
      *(undefined1 *)(iVar4 + 1) = 0x22;
      *(byte *)(iVar4 + 3) = bVar3;
      break;
    case 5:
      *(undefined1 *)(iVar4 + 1) = 0x21;
      *(byte *)(iVar4 + 3) = bVar3;
      break;
    default:
      goto switchD_0042fd31_caseD_6;
    }
    goto switchD_0042fb0e_default;
  }
  if ((int)*(short *)(iVar4 + 0x20) % 10 == 0) {
    switch(*(undefined1 *)(iVar4 + 2)) {
    case 1:
    case 7:
      goto switchD_0042fda6_caseD_1;
    case 2:
      goto switchD_0042fda6_caseD_2;
    case 3:
      goto switchD_0042fda6_caseD_3;
    case 4:
      goto switchD_0042fda6_caseD_4;
    case 5:
      goto switchD_0042fda6_caseD_5;
    case 8:
      goto switchD_0042fda6_caseD_8;
    case 10:
      goto switchD_0042fda6_caseD_a;
    case 0xb:
      goto switchD_0042fda6_caseD_b;
    }
  }
  else {
    switch(*(undefined1 *)(iVar4 + 2)) {
    case 1:
    case 7:
switchD_0042fda6_caseD_1:
      *(undefined1 *)(iVar4 + 1) = 5;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 2:
switchD_0042fda6_caseD_2:
      *(undefined1 *)(iVar4 + 1) = 4;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 3:
switchD_0042fda6_caseD_3:
      *(undefined1 *)(iVar4 + 1) = 3;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 4:
switchD_0042fda6_caseD_4:
      *(undefined1 *)(iVar4 + 1) = 2;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 5:
switchD_0042fda6_caseD_5:
      *(undefined1 *)(iVar4 + 1) = 1;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 8:
switchD_0042fda6_caseD_8:
      *(undefined1 *)(iVar4 + 1) = 0x40;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 10:
switchD_0042fda6_caseD_a:
      *(undefined1 *)(iVar4 + 1) = 0x20;
      *(byte *)(iVar4 + 3) = bVar3;
      goto switchD_0042fb0e_default;
    case 0xb:
switchD_0042fda6_caseD_b:
      *(undefined1 *)(iVar4 + 1) = 0x67;
    }
  }
switchD_0042fd31_caseD_6:
  *(byte *)(iVar4 + 3) = bVar3;
switchD_0042fb0e_default:
  uVar5 = *(uint *)(iVar4 + 8);
  if ((((uVar5 & 4) != 0) && ((uVar5 & 0x800000) != 0)) &&
     ((*(char *)(iVar4 + 2) == '\x01' || ((*(char *)(iVar4 + 2) == '\a' || ((uVar5 & 8) != 0)))))) {
    *(uint *)(iVar4 + 8) = uVar5 & 0xff7fffff;
    FUN_0044f570(*(undefined4 *)(param_1 + 4),param_2,param_3);
  }
  uVar11 = FUN_00477f70();
  iVar7 = local_3c;
  if ((uVar11 & 0x8000) == 0) {
    iVar7 = 0;
  }
  if (iVar7 == 0) {
    FUN_0044f230(*(undefined4 *)(param_1 + 4),param_2,param_3,*(undefined1 *)(iVar4 + 1),
                 *(undefined1 *)(iVar4 + 3));
  }
  else {
    FUN_0044e6d0();
  }
  if (*(int *)(param_1 + 0x2c) != 0) {
    FUN_00433380(param_1,param_2,param_3);
  }
  return;
}


// ==== SavReadSlot @ 00417a90 ====

undefined4 SavReadSlot(uint param_1,undefined4 *param_2,int param_3)

{
  uint uVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 *puVar4;
  undefined4 local_68;
  undefined1 local_64 [100];
  
  uVar1 = param_1;
  if (param_1 < 6) {
    FUN_0048d9e0(local_64,s__s__i_sav_004a2774,s_Saves_004a2780,param_1);
    iVar2 = FUN_0047f6e0(local_64,&DAT_004a2770);
    puVar4 = param_2;
    if (iVar2 != 0) {
      param_1 = 0;
      local_68 = 0;
      FUN_0047f980(param_2,0xb0,1,iVar2);
      FUN_004171c0(DAT_00556ff8,&LAB_00417240,&param_1);
      FUN_00416e70();
      FUN_004171c0(DAT_00556ffc,&LAB_00417240,&local_68);
      FUN_00416e70();
      if ((puVar4[6] != param_1) && (param_3 == 0)) {
        FUN_00417bd0(uVar1,0);
        for (iVar2 = 0x2e; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = 0;
          puVar4 = puVar4 + 1;
        }
        return 0;
      }
      uVar3 = FUN_0048dbd0(puVar4[6] * 0x3190);
      puVar4[0xd] = uVar3;
      FUN_0047f980(uVar3,0x3190,puVar4[6],iVar2);
      FUN_0047fac0(iVar2);
      return 1;
    }
    if (param_3 == 0) {
      FUN_00417bd0(uVar1,0);
    }
  }
  return 0;
}


// ==== SavWriteSlot @ 00417bd0 ====

undefined4
SavWriteSlot(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
            undefined4 param_5,undefined4 param_6,uint param_7)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 in_stack_00000120;
  uint in_stack_000032b4;
  int in_stack_000032b8;
  
  FUN_0048e250();
  if (in_stack_000032b4 < 6) {
    FUN_0048d9e0(&stack0x000000bc,s__s__i_sav_004a2774,s_Saves_004a2780,in_stack_000032b4);
    uVar3 = 0;
    if (in_stack_000032b8 == 0) {
      FUN_0047f6a0(s_Saves_004a2780);
      iVar1 = FUN_0047f6e0(&stack0x000000bc,&DAT_004a2794);
      if (iVar1 != 0) {
        param_1 = 0;
        puVar4 = &param_2;
        for (iVar2 = 0x2d; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = 0;
          puVar4 = puVar4 + 1;
        }
        in_stack_00000120 = 0;
        param_3 = 7;
        puVar4 = (undefined4 *)&stack0x00000124;
        for (iVar2 = 0xc63; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = 0;
          puVar4 = puVar4 + 1;
        }
        param_2 = 5;
        param_5 = 5;
        param_4 = 1;
        param_7 = 0;
        FUN_004171c0(DAT_00556ff8,&LAB_00417240,&param_7);
        FUN_00416e70();
        FUN_0047fa50(&param_1,0xb4,1,iVar1);
        if (param_7 != 0) {
          do {
            FUN_0047fa50(&stack0x00000120,0x3190,1,iVar1);
            uVar3 = uVar3 + 1;
          } while (uVar3 < param_7);
        }
        FUN_0047fac0(iVar1);
        return 1;
      }
    }
    else {
      FUN_004171c0(DAT_00556ff8,&LAB_00417240,&stack0x00000000);
      FUN_00416e70();
      if (*(int *)(in_stack_000032b8 + 0x18) != 0) {
        SavWriteSlot(in_stack_000032b4,0);
        return 1;
      }
      iVar1 = FUN_0047f6e0(&stack0x000000bc,&DAT_004a2794);
      if (iVar1 != 0) {
        FUN_0047fa50(in_stack_000032b8,0xb0,1,iVar1);
        if (*(int *)(in_stack_000032b8 + 0x18) != 0) {
          FUN_0047fa50(*(undefined4 *)(in_stack_000032b8 + 0x34),0x3190,
                       *(int *)(in_stack_000032b8 + 0x18),iVar1);
        }
        FUN_0047fac0(iVar1);
        FUN_0048d9e0(&stack0x000000bc,s__s__i_osf_004a2788,s_Saves_004a2780,in_stack_000032b4);
        FUN_004593e0(&stack0x000000bc);
        return 1;
      }
    }
  }
  return 0;
}


// ==== SavRefreshSlots @ 00417df0 ====

void SavRefreshSlots(void)

{
  int iVar1;
  undefined *puVar2;
  int iVar3;
  
  iVar3 = 0;
  puVar2 = &DAT_00557080;
  do {
    if ((DAT_00505f84 & 0x400) == 0) {
      iVar1 = SavReadSlot(iVar3,puVar2);
      if (iVar1 == 0) goto LAB_00417e23;
    }
    else {
      SavWriteSlot(iVar3,0);
LAB_00417e23:
      SavReadSlot(iVar3,puVar2,0);
    }
    puVar2 = puVar2 + 0xb8;
    iVar3 = iVar3 + 1;
    if ((undefined *)0x5574cf < puVar2) {
      return;
    }
  } while( true );
}


// ==== SavSlotCtx @ 00417e50 ====

undefined * SavSlotCtx(int param_1)

{
  if ((param_1 < 6) && (-1 < param_1)) {
    return &DAT_00557080 + param_1 * 0xb8;
  }
  return (undefined *)0x0;
}


// ==== SavBuildCtx @ 00417e70 ====

undefined4 SavBuildCtx(void)

{
  undefined4 uVar1;
  
  if (DAT_005574d0 != -1) {
    uVar1 = SavSlotCtx(DAT_005574d0);
    return uVar1;
  }
  return 0;
}


// ==== SavGetSlot @ 00417e90 ====

undefined4 SavGetSlot(void)

{
  return DAT_005574d0;
}


// ==== SavSetSlot @ 00417ea0 ====

void SavSetSlot(undefined4 param_1)

{
  DAT_005574d0 = param_1;
  return;
}


// ==== SavMarkRecord @ 00417eb0 ====

void SavMarkRecord(uint param_1)

{
  int iVar1;
  int iVar2;
  uint *puVar3;
  
  iVar1 = SavBuildCtx();
  if (iVar1 != 0) {
    iVar2 = FUN_00416d60();
    if (iVar2 == 0) {
      if (7 < param_1) {
        if (*(uint *)(iVar1 + 0x18) < param_1) {
          return;
        }
        *(uint *)(param_1 * 0x3190 + *(int *)(iVar1 + 0x34)) =
             *(uint *)(param_1 * 0x3190 + *(int *)(iVar1 + 0x34)) | 1;
        return;
      }
    }
    else if (7 < param_1) {
      return;
    }
    puVar3 = (uint *)(*(int *)(iVar1 + 0x34) + param_1 * 0x3190);
    *puVar3 = *puVar3 | 1;
    puVar3 = (uint *)(param_1 * 0x3190 + *(int *)(iVar1 + 0x34));
    *puVar3 = *puVar3 | 2;
  }
  return;
}


// ==== SavWriteRecord @ 00417f40 ====

undefined4 SavWriteRecord(int param_1,undefined4 *param_2)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  
  iVar1 = SavBuildCtx();
  if ((iVar1 != 0) && (param_1 < *(int *)(iVar1 + 0x18))) {
    puVar3 = (undefined4 *)(param_1 * 0x3190 + 4 + *(int *)(iVar1 + 0x34));
    for (iVar2 = 0xc63; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = *param_2;
      param_2 = param_2 + 1;
      puVar3 = puVar3 + 1;
    }
    return 1;
  }
  return 0;
}


// ==== SavRecordPtr @ 00417f90 ====

int SavRecordPtr(int param_1)

{
  int iVar1;
  
  iVar1 = SavBuildCtx();
  if ((iVar1 != 0) && (param_1 < *(int *)(iVar1 + 0x18))) {
    return param_1 * 0x3190 + 4 + *(int *)(iVar1 + 0x34);
  }
  return 0;
}


// ==== SavCommit @ 00417fc0 ====

void SavCommit(void)

{
  undefined4 uVar1;
  
  uVar1 = SavBuildCtx();
  SavWriteSlot(DAT_005574d0,uVar1);
  return;
}


// ==== SavTreeLoad @ 004173e0 ====

void SavTreeLoad(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                undefined4 param_5)

{
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  local_10 = param_3;
  local_c = param_4;
  local_14 = param_2;
  local_8 = 0;
  local_4 = param_5;
  FUN_004172f0(param_1,&local_14,1);
  FUN_00416e70();
  return;
}


// ==== SavTreeWalk2 @ 004172f0 ====

void SavTreeWalk2(int *param_1,int *param_2,int param_3)

{
  uint *puVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar4 = 0;
  if ((param_1 != (int *)0x0) && (param_1[4] == 0)) {
    if ((1 < DAT_00505f88) || ((DAT_00505f84 & 0x20000) != 0)) {
      param_2[4] = 0;
    }
    param_1[4] = 1;
    if (((*param_2 == 0) || (1 < DAT_00505f88)) || ((DAT_00505f84 & 0x20000) != 0)) {
      uVar3 = (uint)(param_2[4] == 0);
    }
    else {
      uVar3 = *(uint *)(*param_1 * 0x3190 + *param_2) & 1;
    }
    if (((uVar3 == 0) && (param_3 == 0)) &&
       (puVar1 = (uint *)(*(int *)param_2[2] + param_2[3] * 0x20),
       *(int *)(*(int *)param_2[2] + 0x1c + param_2[3] * 0x20) == 0)) {
      uVar2 = *puVar1 & 0xfffffffb;
    }
    else {
      puVar1 = (uint *)(*(int *)param_2[2] + param_2[3] * 0x20);
      uVar2 = *(uint *)(*(int *)param_2[2] + param_2[3] * 0x20) | 4;
    }
    *puVar1 = uVar2;
    param_2[3] = param_2[3] + 1;
    if (param_1[2] != 0) {
      do {
        SavTreeWalk2(*(undefined4 *)(param_1[1] + uVar4 * 4),param_2,uVar3);
        uVar4 = uVar4 + 1;
      } while (uVar4 < (uint)param_1[2]);
    }
  }
  return;
}


// ==== SavTreeWalk @ 004171c0 ====

undefined4 SavTreeWalk(int param_1,code *param_2,undefined4 param_3)

{
  int iVar1;
  uint uVar2;
  
  if ((param_1 != 0) && (*(int *)(param_1 + 0x10) == 0)) {
    *(undefined4 *)(param_1 + 0x10) = 1;
    iVar1 = (*param_2)(param_1,param_3);
    if (iVar1 != 0) {
      return 1;
    }
    if ((*(int *)(param_1 + 4) != 0) && (uVar2 = 0, *(int *)(param_1 + 8) != 0)) {
      do {
        iVar1 = SavTreeWalk(*(undefined4 *)(*(int *)(param_1 + 4) + uVar2 * 4),param_2,param_3);
        if (iVar1 != 0) {
          return 1;
        }
        uVar2 = uVar2 + 1;
      } while (uVar2 < *(uint *)(param_1 + 8));
      return 0;
    }
  }
  return 0;
}


// ==== SavClearVisited @ 00416e70 ====

void SavClearVisited(void)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  if (0 < DAT_00556fe4) {
    do {
      iVar1 = *(int *)(DAT_00556ff0 + iVar2 * 4);
      if (iVar1 != 0) {
        *(undefined4 *)(iVar1 + 0x10) = 0;
        *(undefined4 *)(DAT_00556ff4 + iVar2 * 4) = 0;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00556fe4);
  }
  iVar2 = 0;
  if (0 < DAT_00556fd0) {
    do {
      iVar1 = *(int *)(DAT_00556fdc + iVar2 * 4);
      if (iVar1 != 0) {
        *(undefined4 *)(iVar1 + 0x10) = 0;
        *(undefined4 *)(DAT_00556fe0 + iVar2 * 4) = 0;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_00556fd0);
  }
  return;
}


// ==== SavMenuList @ 004179b0 ====

void SavMenuList(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                undefined4 param_5,undefined4 param_6,undefined4 param_7)

{
  undefined4 uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;
  
  uVar1 = param_4;
  local_10 = param_3;
  local_c = param_4;
  local_8 = 0;
  local_4 = param_5;
  param_3 = 0;
  SavTreeWalk(param_2,&LAB_00417240,&param_3);
  SavClearVisited();
  uVar2 = FUN_004105f0(&DAT_00557010 + param_1 * 4,&DAT_004b8a14,&DAT_004b8a14,0,0,0,param_3,0,0,0,0
                       ,0,param_7,param_6);
  uVar3 = FUN_00410d50(s_Levels____004a2764,0,0,0,0,6,1,uVar2,0);
  FUN_00411260(uVar1,uVar3);
  local_8 = uVar2;
  SavTreeWalk(param_2,&LAB_004177e0,&local_10);
  SavClearVisited();
  return;
}


// ==== LevelNodeGet @ 00417090 ====

int * LevelNodeGet(undefined4 param_1,int param_2)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  undefined4 local_3c [15];
  
  if (param_2 != 0) {
    iVar1 = FUN_00416f40(param_1,param_2);
    if (iVar1 != 0) {
      piVar2 = (int *)FUN_00417020(param_1,param_2);
      return piVar2;
    }
    piVar2 = (int *)FUN_0048dbd0(0x14);
    FUN_00416ed0(param_1,param_2,1);
    FUN_00416fb0(param_1,param_2,piVar2);
    if (piVar2 != (int *)0x0) {
      iVar1 = FUN_00416e20(param_1,param_2);
      if (iVar1 != -1) {
        uVar3 = FUN_00478f50(DAT_005560c4,param_2,s_LevelLinks_004a26e8,0);
        iVar4 = FUN_00479050(DAT_005560c0,uVar3);
        *piVar2 = iVar1;
        uVar6 = 0;
        if (iVar4 == 0) {
          piVar2[1] = 0;
        }
        else {
          uVar5 = FUN_00477440(iVar4,local_3c,&DAT_004a13e4);
          if (uVar5 == 0) goto LAB_0041718d;
          iVar1 = FUN_0048dbd0(uVar5 * 4);
          piVar2[1] = iVar1;
          if (uVar5 != 0) {
            do {
              uVar3 = LevelNodeGet(param_1,local_3c[uVar6]);
              *(undefined4 *)(piVar2[1] + uVar6 * 4) = uVar3;
              uVar6 = uVar6 + 1;
            } while (uVar6 < uVar5);
            piVar2[2] = uVar6;
            return piVar2;
          }
        }
        piVar2[2] = 0;
        return piVar2;
      }
LAB_0041718d:
      FUN_0048db80(piVar2);
      return (int *)0x0;
    }
  }
  return (int *)0x0;
}


// ==== ListHasItem @ 00416f40 ====

undefined4 ListHasItem(int *param_1,byte *param_2)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  bool bVar7;
  
  iVar4 = 0;
  if (*param_1 < 1) {
    return 0;
  }
  puVar6 = (undefined4 *)param_1[1];
  do {
    pbVar2 = (byte *)*puVar6;
    pbVar5 = param_2;
    do {
      bVar1 = *pbVar2;
      bVar7 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_00416f7d:
        iVar3 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
        goto LAB_00416f82;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar7 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_00416f7d;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00416f82:
    if (iVar3 == 0) {
      return *(undefined4 *)(param_1[4] + iVar4 * 4);
    }
    iVar4 = iVar4 + 1;
    puVar6 = puVar6 + 1;
    if (*param_1 <= iVar4) {
      return 0;
    }
  } while( true );
}


// ==== ListGetNode @ 00417020 ====

undefined4 ListGetNode(int *param_1,byte *param_2)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  bool bVar7;
  
  iVar4 = 0;
  if (*param_1 < 1) {
    return 0;
  }
  puVar6 = (undefined4 *)param_1[1];
  do {
    pbVar2 = (byte *)*puVar6;
    pbVar5 = param_2;
    do {
      bVar1 = *pbVar2;
      bVar7 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_0041705d:
        iVar3 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
        goto LAB_00417062;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar7 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_0041705d;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00417062:
    if (iVar3 == 0) {
      return *(undefined4 *)(param_1[3] + iVar4 * 4);
    }
    iVar4 = iVar4 + 1;
    puVar6 = puVar6 + 1;
    if (*param_1 <= iVar4) {
      return 0;
    }
  } while( true );
}


// ==== ListAddName @ 00416ed0 ====

void ListAddName(int *param_1,byte *param_2,undefined4 param_3)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  bool bVar7;
  
  iVar4 = 0;
  if (*param_1 < 1) {
    return;
  }
  puVar6 = (undefined4 *)param_1[1];
  do {
    pbVar2 = (byte *)*puVar6;
    pbVar5 = param_2;
    do {
      bVar1 = *pbVar2;
      bVar7 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_00416f0d:
        iVar3 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
        goto LAB_00416f12;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar7 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_00416f0d;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00416f12:
    if (iVar3 == 0) {
      *(undefined4 *)(param_1[4] + iVar4 * 4) = param_3;
      return;
    }
    iVar4 = iVar4 + 1;
    puVar6 = puVar6 + 1;
    if (*param_1 <= iVar4) {
      return;
    }
  } while( true );
}


// ==== ListSetNode @ 00416fb0 ====

void ListSetNode(int *param_1,byte *param_2,undefined4 param_3)

{
  byte bVar1;
  byte *pbVar2;
  int iVar3;
  int iVar4;
  byte *pbVar5;
  undefined4 *puVar6;
  bool bVar7;
  
  iVar4 = 0;
  if (*param_1 < 1) {
    return;
  }
  puVar6 = (undefined4 *)param_1[1];
  do {
    pbVar2 = (byte *)*puVar6;
    pbVar5 = param_2;
    do {
      bVar1 = *pbVar2;
      bVar7 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_00416fed:
        iVar3 = (1 - (uint)bVar7) - (uint)(bVar7 != 0);
        goto LAB_00416ff2;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar2[1];
      bVar7 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_00416fed;
      pbVar2 = pbVar2 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_00416ff2:
    if (iVar3 == 0) {
      *(undefined4 *)(param_1[3] + iVar4 * 4) = param_3;
      return;
    }
    iVar4 = iVar4 + 1;
    puVar6 = puVar6 + 1;
    if (*param_1 <= iVar4) {
      return;
    }
  } while( true );
}


// ==== SaveMenuBuild @ 00415a10 ====

void SaveMenuBuild(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  int iVar7;
  undefined *puVar8;
  undefined1 local_64 [100];
  
  DAT_00556ff8 = LevelNodeGet(&DAT_00556fe4,*DAT_00556fe8);
  DAT_00556ffc = LevelNodeGet(&DAT_00556fd0,*DAT_00556fd4);
  SavClearVisited();
  iVar1 = FUN_00414c90(DAT_00556fc4,s_Levels_004a2574);
  iVar2 = FUN_00414c90(DAT_00556fc4,s_Tutorials_004a2568);
  iVar3 = FUN_00414c90(DAT_00556fc4,s_Load_Level_Save_004a2558);
  uVar4 = FUN_004105f0(&DAT_00557028,&DAT_004b8a14,&DAT_004b8a14,0xffffff38,0xffffff9c,0x3c,5,
                       0xffffffec,200,0xffffffec,0xffffffd8,0,FUN_004150b0,
                       *(undefined4 *)(*DAT_00556fc4 + iVar1 * 4));
  iVar3 = iVar3 * 4;
  iVar7 = **(int **)(*(int *)(*DAT_00556fc4 + iVar3) + 0x18);
  uVar5 = FUN_00410d50(&DAT_004b8a14,*(undefined4 *)(iVar7 + 0xc),*(undefined4 *)(iVar7 + 8),0,0,6,1
                       ,uVar4,0);
  FUN_00411260(*(undefined4 *)(*DAT_00556fc4 + iVar3),uVar5);
  uVar5 = FUN_004105f0(&DAT_00557028,&DAT_004b8a14,&DAT_004b8a14,0xffffff38,0xffffff9c,0x3c,5,
                       0xffffffec,200,0xffffffec,0xffffffd8,0,FUN_004150b0,0);
  iVar7 = **(int **)(*(int *)(*DAT_00556fc4 + iVar3) + 0x18);
  uVar6 = FUN_00410d50(&DAT_004b8a14,*(undefined4 *)(iVar7 + 0xc),*(undefined4 *)(iVar7 + 8),0,0,6,1
                       ,uVar5,0);
  FUN_00411260(*(undefined4 *)*DAT_00556fcc,uVar6);
  iVar7 = 0;
  puVar8 = &DAT_00557080;
  do {
    iVar7 = iVar7 + 1;
    uVar6 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_Save_Game_004a254c,0,iVar7);
    uVar6 = FUN_004790b0(DAT_005560c0,uVar6);
    FUN_0048d9e0(local_64,s__s__i_004a253c,uVar6);
    FUN_00413f00(local_64,0x5f,0x20);
    FUN_004152c0(puVar8,local_64);
    FUN_00410740(uVar4,local_64,1,
                 *(undefined4 *)(**(int **)(*(int *)(*DAT_00556fc4 + iVar3) + 0x18) + 8),0,0,0);
    FUN_00410740(uVar5,local_64,1,
                 *(undefined4 *)(**(int **)(*(int *)(*DAT_00556fc4 + iVar3) + 0x18) + 8),0,0,0);
    puVar8 = puVar8 + 0xb8;
  } while ((int)puVar8 < 0x557418);
  SavRefreshSlots();
  uVar4 = SavBuildCtx(0,&LAB_00417460);
  SavMenuList(4,DAT_00556ff8,&DAT_00556fe4,*(undefined4 *)(*DAT_00556fc4 + iVar1 * 4),uVar4);
  uVar4 = SavBuildCtx(0,&LAB_00417700);
  SavMenuList(5,DAT_00556ffc,&DAT_00556fd0,*(undefined4 *)(*DAT_00556fc4 + iVar2 * 4),uVar4);
  return;
}


// ==== LoadMenuBuild @ 004161f0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void LoadMenuBuild(undefined4 param_1)

{
  char cVar1;
  undefined4 uVar2;
  char *pcVar3;
  int iVar4;
  undefined *puVar5;
  uint uVar6;
  uint uVar7;
  undefined4 *puVar8;
  char *pcVar9;
  
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_MenuWipe_004a26d0,0);
  pcVar3 = (char *)FUN_00479050(DAT_005560c0,uVar2);
  if (pcVar3 == (char *)0x0) {
    uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s__MenuWipe_004a26c4,0);
    pcVar3 = (char *)FUN_00479050(DAT_005560c0,uVar2);
    if (pcVar3 == (char *)0x0) goto LAB_004162aa;
    if ((DAT_00505f84 & 0x400000) == 0) {
      iVar4 = __strnicmp(pcVar3,&DAT_004a23f4,4);
      goto joined_r0x00416288;
    }
  }
  else {
    iVar4 = __strnicmp(pcVar3,&DAT_004a23f4,4);
joined_r0x00416288:
    if (iVar4 != 0) {
      DAT_005574ec = FUN_00472cd0(0,pcVar3,&PTR_DAT_004a13e0,1);
    }
  }
  FUN_0048db80(pcVar3);
LAB_004162aa:
  DAT_00557594 = (undefined4 *)FUN_0048dbd0(0x728);
  puVar8 = DAT_00557594;
  for (iVar4 = 0x1ca; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  FUN_00415d00(param_1,s_Menu__SaveText_004a1f10,DAT_00557594);
  DAT_00557598 = (undefined4 *)FUN_0048dbd0(0x728);
  puVar8 = DAT_00557598;
  for (iVar4 = 0x1ca; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  FUN_00415d00(param_1,s_Menu__LevelText_004a1f20,DAT_00557598);
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_Save_Game_004a254c,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_00557554;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
    FUN_0048db80(iVar4);
  }
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_Load_Game_004a26b8,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_00557514;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
    FUN_0048db80(iVar4);
  }
  uVar2 = FUN_00478f50(DAT_005560c4,s_Menu__Overwrite_004a1f30,s_Title_004a24b8,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_0055759c;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
    FUN_0048db80(iVar4);
  }
  uVar2 = FUN_00478f50(DAT_005560c4,s_Menu__Overwrite_004a1f30,&DAT_004a26b0,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_0055761c;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
    FUN_0048db80(iVar4);
  }
  uVar2 = FUN_00478f50(DAT_005560c4,s_Menu__Overwrite_004a1f30,&DAT_004a26ac,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_0055771c;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
  }
  uVar2 = FUN_00478f50(DAT_005560c4,s_Menu__Overwrite_004a1f30,s_Cancel_004a26a4,0);
  iVar4 = FUN_00479050(DAT_005560c0,uVar2);
  if (iVar4 != 0) {
    pcVar3 = (char *)FUN_004102b0(iVar4);
    uVar6 = 0xffffffff;
    do {
      pcVar9 = pcVar3;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar9 = pcVar3 + 1;
      cVar1 = *pcVar3;
      pcVar3 = pcVar9;
    } while (cVar1 != '\0');
    uVar6 = ~uVar6;
    pcVar3 = pcVar9 + -uVar6;
    pcVar9 = (char *)&DAT_0055779c;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *(undefined4 *)pcVar9 = *(undefined4 *)pcVar3;
      pcVar3 = pcVar3 + 4;
      pcVar9 = pcVar9 + 4;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *pcVar9 = *pcVar3;
      pcVar3 = pcVar3 + 1;
      pcVar9 = pcVar9 + 1;
    }
  }
  DAT_00557078 = DAT_00556138;
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a24e4,s_Version_004a269c,0);
  DAT_0055707c = FUN_00479050(DAT_005560c0,uVar2);
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_DefaultLevelBMPS_004a2688,0);
  DAT_005574d4 = FUN_00479050(DAT_005560c0,uVar2);
  DAT_005574fc = FUN_004734f0(DAT_005560d8,3,0x3f4ccccd,0x3f4ccccd,0x3f4ccccd);
  FUN_00473b40(DAT_005574fc,1);
  DAT_00556fc4 = FUN_004140c0(param_1,s_MainMenuFull_004a2678,&DAT_00557010,FUN_004103d0,
                              &DAT_0055701c,0,0xffffffff);
  DAT_00556fcc = FUN_004140c0(param_1,s_SaveMenu_004a266c,&DAT_00557060,&LAB_00415190,0xffffffff);
  FUN_004154b0(DAT_005560c0,&DAT_00556fe4,s_StartLevel_004a2660);
  FUN_004154b0(DAT_005560c0,&DAT_00556fd0,s_TutorialStartLevel_004a264c);
  SaveMenuBuild(DAT_00556fc4);
  DAT_00556fc0 = FUN_004140c0(param_1,s_PausedMenu_004a2640,&DAT_0055702c,0,&DAT_00557030,
                              &LAB_00414f20,&DAT_00557034,FUN_00414de0,&DAT_00557038,FUN_00414df0,
                              &DAT_0055703c,FUN_00414d30,&DAT_00557040,&LAB_00414ee0,&DAT_00557044,
                              &LAB_00414f10,&DAT_00557048,0,&DAT_0055704c,0,&DAT_00557050,
                              &LAB_00414e50,&DAT_00557054,&LAB_00414ea0,&DAT_00557058,&LAB_00414ec0,
                              &DAT_0055705c,&LAB_00414e80,0xffffffff);
  DAT_00556fc8 = FUN_004140c0(param_1,s_OptionsMenu_004a2634,&DAT_00557030,&LAB_00414f20,
                              &DAT_00557034,FUN_00414de0,&DAT_00557038,FUN_00414df0,&DAT_0055703c,
                              FUN_00414d30,&DAT_00557040,&LAB_00414ee0,&DAT_00557044,&LAB_00414f10,
                              &DAT_0055702c,0,0xffffffff);
  DAT_00557034 = 8;
  _DAT_0055702c = 0;
  DAT_0055703c = 5;
  _DAT_00557040 = 1;
  DAT_00557044 = 0;
  DAT_00557030 = FUN_00415030();
  _DAT_00557038 = FUN_00414e10();
  DAT_00557500 = 0;
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_MaxLevelScreens_004a2624,0);
  iVar4 = FUN_004790b0(param_1,uVar2);
  if (iVar4 == 0) {
    puVar5 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_MaxLevelScreens_004a2624,0);
    puVar5 = (undefined *)FUN_004790b0(param_1,uVar2);
  }
  _DAT_00557828 = FUN_0048db70(puVar5);
  if (_DAT_00557828 == 0) {
    _DAT_00557828 = 1;
  }
  _DAT_00557824 = 0;
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_SaveImage_004a260c,s_BigWidth_004a2618,0);
  iVar4 = FUN_004790b0(param_1,uVar2);
  if (iVar4 == 0) {
    puVar5 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_SaveImage_004a260c,s_BigWidth_004a2618,0);
    puVar5 = (undefined *)FUN_004790b0(param_1,uVar2);
  }
  DAT_0055750c = FUN_0048db70(puVar5);
  if (DAT_0055750c == 0) {
    DAT_0055750c = 0x50;
  }
  uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_SaveImage_004a260c,s_BigHeight_004a2600,0);
  iVar4 = FUN_004790b0(param_1,uVar2);
  if (iVar4 == 0) {
    puVar5 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_005560c4,&DAT_004a2544,s_SaveImage_004a260c,s_BigHeight_004a2600,0);
    puVar5 = (undefined *)FUN_004790b0(param_1,uVar2);
  }
  DAT_00557510 = FUN_0048db70(puVar5);
  if (DAT_00557510 == 0) {
    DAT_00557510 = 0x3c;
  }
  FUN_00418110(1);
  return;
}


// ==== IsInGame @ 00416d60 ====

bool IsInGame(void)

{
  return DAT_00557024 != -1;
}


// ==== ActiveObjectList @ 00416e00 ====

undefined4 * ActiveObjectList(void)

{
  int iVar1;
  undefined4 *puVar2;
  
  iVar1 = IsInGame();
  puVar2 = &DAT_00556fd0;
  if (iVar1 == 0) {
    puVar2 = &DAT_00556fe4;
  }
  return puVar2;
}


// ==== ListFindName @ 00416e20 ====

int ListFindName(int *param_1,char *param_2)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  if (*param_1 < 1) {
    return -1;
  }
  do {
    iVar1 = __strcmpi(param_2,*(char **)(param_1[1] + iVar2 * 4));
    if (iVar1 == 0) {
      return iVar2;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < *param_1);
  return -1;
}


// ==== CheckpointStep @ 00462270 ====

void CheckpointStep(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 uVar5;
  uint uVar6;
  
  puVar1 = (undefined4 *)FUN_00431560();
  uVar2 = ActiveObjectList(*puVar1);
  uVar2 = ListFindName(uVar2);
  iVar3 = SavGetSlot();
  if (iVar3 == -1) {
    SavSetSlot(5);
  }
  iVar4 = FUN_00431560();
  if (*(int *)(iVar4 + 0x27c) == 1) {
    SavMarkRecord(uVar2);
  }
  uVar5 = FUN_00460200();
  SavWriteRecord(uVar2,uVar5);
  if (iVar3 == -1) {
    iVar3 = SavSlotCtx(5);
    if (iVar3 != 0) {
      puVar1 = (undefined4 *)(*(int *)(iVar3 + 0x34) + 0x3190);
      for (uVar6 = *(int *)(iVar3 + 0x18) * 0x3190 - 0x3190U >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar1 = 0;
        puVar1 = puVar1 + 1;
      }
      for (iVar3 = 0; iVar3 != 0; iVar3 = iVar3 + -1) {
        *(undefined1 *)puVar1 = 0;
        puVar1 = (undefined4 *)((int)puVar1 + 1);
      }
      iVar3 = FUN_00431560();
      if (*(int *)(iVar3 + 0x27c) == 1) {
        SavMarkRecord(uVar2);
      }
      uVar5 = FUN_00460200();
      SavWriteRecord(uVar2,uVar5);
    }
    SavCommit();
    SavReadSlot(5,&DAT_00557418,0);
  }
  iVar3 = SavBuildCtx(&DAT_00556fe4,
                      *(undefined4 *)
                       (*(int *)(*(int *)(*(int *)(*DAT_00556fc4 + 4) + 0x18) + 4) + 0x10),0);
  SavTreeLoad(DAT_00556ff8,*(undefined4 *)(iVar3 + 0x34));
  FUN_00418110(0);
  return;
}


// ==== SavObjSerialize @ 004177e0 ====

undefined4 SavObjSerialize(int *param_1,int *param_2)

{
  char cVar1;
  undefined4 uVar2;
  int iVar3;
  undefined *puVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  char *pcVar11;
  char *pcVar12;
  char acStack_400 [1024];
  
  uVar7 = *(undefined4 *)(*(int *)(*param_2 + 4) + *param_1 * 4);
  uVar2 = FUN_00478f50(DAT_005560c4,uVar7,s_FrontEndX_004a2758,0);
  iVar3 = FUN_004790b0(DAT_005560c0,uVar2);
  if (iVar3 == 0) {
    puVar4 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_005560c4,uVar7,s_FrontEndX_004a2758,0);
    puVar4 = (undefined *)FUN_004790b0(DAT_005560c0,uVar2);
  }
  uVar2 = FUN_0048db70(puVar4);
  uVar5 = FUN_00478f50(DAT_005560c4,uVar7,s_FrontEndY_004a274c,0);
  iVar3 = FUN_004790b0(DAT_005560c0,uVar5);
  if (iVar3 == 0) {
    puVar4 = &DAT_004b8a14;
  }
  else {
    uVar5 = FUN_00478f50(DAT_005560c4,uVar7,s_FrontEndY_004a274c,0);
    puVar4 = (undefined *)FUN_004790b0(DAT_005560c0,uVar5);
  }
  uVar5 = FUN_0048db70(puVar4);
  uVar6 = FUN_00478f50(DAT_005560c4,uVar7,s_FrontEndOpen_004a273c,0);
  iVar3 = FUN_004790d0(DAT_005560c0,uVar6);
  if (iVar3 == 2) {
    iVar3 = 0;
  }
  uVar7 = FUN_00478f50(DAT_005560c4,uVar7,s_MenuBMP_004a2734,0);
  iVar8 = FUN_00479050(DAT_005560c0,uVar7);
  if (iVar8 != 0) {
    FUN_00410740(param_2[2],iVar8,0,0,uVar2,uVar5,iVar3);
    FUN_0048db80(iVar8);
    return 0;
  }
  uVar9 = 0xffffffff;
  pcVar11 = DAT_005574d4;
  do {
    pcVar12 = pcVar11;
    if (uVar9 == 0) break;
    uVar9 = uVar9 - 1;
    pcVar12 = pcVar11 + 1;
    cVar1 = *pcVar11;
    pcVar11 = pcVar12;
  } while (cVar1 != '\0');
  uVar9 = ~uVar9;
  pcVar11 = pcVar12 + -uVar9;
  pcVar12 = acStack_400;
  for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
    *(undefined4 *)pcVar12 = *(undefined4 *)pcVar11;
    pcVar11 = pcVar11 + 4;
    pcVar12 = pcVar12 + 4;
  }
  for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
    *pcVar12 = *pcVar11;
    pcVar11 = pcVar11 + 1;
    pcVar12 = pcVar12 + 1;
  }
  FUN_00410740(param_2[2],acStack_400,0,0,uVar2,uVar5,iVar3);
  return 0;
}


// ==== SavObjCallbackB @ 00417700 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void SavObjCallbackB(float param_1,undefined4 param_2)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  char *pcVar4;
  
  piVar1 = DAT_00557598;
  piVar2 = (int *)FUN_00417280(DAT_00556ffc,param_2);
  pcVar4 = *(char **)(DAT_00556fd8 + *piVar2 * 4);
  DAT_00557838 = DAT_00557834;
  DAT_00557834 = *piVar2 + 1;
  if ((pcVar4 == (char *)0x0) || (*pcVar4 == '\0')) {
    pcVar4 = *(char **)(DAT_00556fd4 + *piVar2 * 4);
  }
  if ((*piVar1 != 0) && (DAT_004db8c4 != 0)) {
    FUN_00485230(*piVar1,pcVar4);
    if (DAT_00557834 != DAT_00557838) {
      DAT_0055783c = 1;
      _DAT_00557840 = 0.0;
    }
    if ((DAT_0055783c != 0) &&
       (_DAT_00557840 = param_1 + _DAT_00557840, _DAT_0049e228 < _DAT_00557840 * _DAT_0049e1a4)) {
      iVar3 = FUN_004161a0(DAT_00557834);
      if (iVar3 != 0) {
        DAT_0055783c = 0;
      }
    }
  }
  DAT_004db8c4 = 0;
  return;
}


// ==== CheckpointBlobBuild @ 0045fda0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 CheckpointBlobBuild(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined *puVar3;
  uint uVar4;
  int iVar5;
  undefined4 *puVar6;
  int *piVar7;
  undefined4 *puVar8;
  float10 fVar9;
  
  puVar1 = (undefined4 *)FUN_00431560();
  DAT_00552994 = (int *)FUN_0048dbd0(0x318c);
  piVar7 = DAT_00552994;
  for (iVar5 = 0xc63; iVar5 != 0; iVar5 = iVar5 + -1) {
    *piVar7 = 0;
    piVar7 = piVar7 + 1;
  }
  uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Enable_004a9488,0);
  iVar5 = FUN_004790d0(DAT_00555b54,uVar2);
  *DAT_00552994 = iVar5;
  if (*DAT_00552994 == 2) {
    *DAT_00552994 = 0;
  }
  uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Modifier_004a947c,0);
  iVar5 = FUN_004790b0(DAT_00555b54,uVar2);
  if (iVar5 == 0) {
    puVar3 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Modifier_004a947c,0);
    puVar3 = (undefined *)FUN_004790b0(DAT_00555b54,uVar2);
  }
  iVar5 = FUN_0048db70(puVar3);
  DAT_00552994[0x70] = iVar5;
  uVar2 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Timer_004a9324,0);
  iVar5 = FUN_004790b0(DAT_00555b54,uVar2);
  if (iVar5 == 0) {
    puVar3 = &DAT_004b8a14;
  }
  else {
    uVar2 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Timer_004a9324,0);
    puVar3 = (undefined *)FUN_004790b0(DAT_00555b54,uVar2);
  }
  fVar9 = (float10)FUN_0048d980(puVar3);
  DAT_0055298c = (float)fVar9;
  if ((float10)_DAT_0049e6b4 == fVar9) {
    DAT_0055298c = 76.9375;
  }
  uVar2 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_DisplayText_004a9470,0);
  DAT_00555bc4 = FUN_004790d0(DAT_00555b54,uVar2);
  if (DAT_00555bc4 == 2) {
    DAT_00555bc4 = 1;
  }
  uVar2 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_DisplayImages_004a9460,0);
  DAT_00555bc8 = FUN_004790d0(DAT_00555b54,uVar2);
  if (DAT_00555bc8 == 2) {
    DAT_00555bc8 = 1;
  }
  uVar2 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_DisplayFlics_004a9450,0);
  DAT_00555bcc = FUN_004790d0(DAT_00555b54,uVar2);
  if (DAT_00555bcc == 2) {
    DAT_00555bcc = 1;
  }
  DAT_00552994[0x73a] = (int)(DAT_00552994 + 0x73b);
  uVar4 = 0;
  DAT_00552994[0x739] = 10;
  do {
    puVar6 = (undefined4 *)(*(int *)(DAT_00552990 + 0x1ce8) + uVar4);
    puVar8 = (undefined4 *)(DAT_00552994[0x73a] + uVar4);
    uVar4 = uVar4 + 0x210;
    for (iVar5 = 0x84; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
  } while (uVar4 < 0x14a0);
  uVar4 = 0;
  do {
    FUN_004600a0(DAT_00552994[0x73a] + uVar4);
    uVar4 = uVar4 + 0x210;
  } while (uVar4 < 0x14a0);
  uVar4 = 0;
  do {
    FUN_00460140(DAT_00552994[0x73a] + uVar4);
    uVar4 = uVar4 + 0x210;
  } while (uVar4 < 0x14a0);
  iVar5 = __strcmpi((char *)*puVar1,s_Levels__Level03_004a9440);
  if (iVar5 == 0) {
    *(undefined4 *)(DAT_00552994[0x73a] + 0x868) = 1;
  }
  return 1;
}


// ==== CheckpointBlobFree @ 004601e0 ====

void CheckpointBlobFree(void)

{
  FUN_0048db80(DAT_00552994);
  return;
}


// ==== CheckpointBlobGet @ 004601f0 ====

undefined4 CheckpointBlobGet(void)

{
  return DAT_00552994;
}


// ==== RewardDisplayTick @ 00461790 ====

void RewardDisplayTick(uint *param_1,uint param_2,uint param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  float local_3c;
  float fStack_38;
  undefined4 local_34;
  float local_30;
  float fStack_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;
  
  if (param_3 == 0) {
    iVar2 = 0;
  }
  else {
    iVar2 = param_3 - 1;
  }
  iVar2 = *(int *)(DAT_00552994 + 0x1ce8) + iVar2 * 0x210;
  if (((DAT_00555bc8 != 0) && ((*param_1 & 2) != 0)) && ((param_2 & 2) != 0)) {
    local_3c = (float)param_1[0x58];
    local_30 = (float)param_1[0x57];
    fStack_38 = 0.0;
    fStack_2c = (float)(uint)local_3c;
    FUN_0047dcb0(param_1[0x80],0,&local_30,0);
  }
  if (((DAT_00555bc8 != 0) && ((*param_1 & 0x40) != 0)) && ((param_2 & 0x40) != 0)) {
    local_30 = (float)param_1[0x5a];
    local_3c = (float)param_1[0x59];
    fStack_2c = 0.0;
    fStack_38 = (float)(uint)local_30;
    FUN_0047dcb0(param_1[0x83],0,&local_3c,0);
  }
  if (((DAT_00555bcc != 0) && ((*param_1 & 4) != 0)) && ((param_2 & 4) != 0)) {
    if (param_1[0x7f] == 0) {
      FUN_00483c80(param_1 + 0x5b,param_1 + 0x7f,0);
    }
    local_10 = (float)param_1[0x7d];
    local_30 = (float)param_1[0x7c];
    local_c = (float)param_1[0x7e];
    local_8 = (float)param_1[0x7b];
    uVar3 = param_1[0x7f];
    fStack_2c = 0.0;
    local_4 = (float)(uint)local_30;
    FUN_00484070(uVar3,&local_10,*(int *)(uVar3 + 0x624) < 0x23,0);
  }
  if (DAT_00555bc4 != 0) {
    if (((*param_1 & 1) != 0) && ((param_2 & 1) != 0)) {
      uVar3 = FUN_00401c50(param_1[0x81],param_1 + 0x35);
      FUN_0047a180(param_1[0x81],param_1[0x55] - (uVar3 >> 1),param_1[0x56],param_1 + 0x35);
      if (param_3 < 10) {
        iVar1 = iVar2 + 0x54;
        uVar3 = FUN_00401c50(DAT_00555b48,iVar1);
        FUN_0047a180(DAT_00555b48,DAT_00555b9c - (uVar3 >> 1),DAT_00555ba0,iVar1);
      }
    }
    if ((DAT_00555bc4 != 0) &&
       ((((*param_1 & 0x10) != 0 && ((param_2 & 0x10) != 0)) ||
        (((*param_1 & 0x20) != 0 && ((param_2 & 0x20) != 0)))))) {
      local_1c = 0x3df0f0f1;
      local_18 = 0x3df0f0f1;
      local_14 = 0x3df0f0f1;
      local_28 = 0x3e70f0f1;
      local_24 = 0x3e70f0f1;
      local_20 = 0x3e70f0f1;
      local_3c = 0.0;
      fStack_38 = 0.0;
      local_34 = 0;
      uVar3 = FUN_00401c50(param_1[0x81],param_1 + 0x35);
      local_30 = (float)(uVar3 + 0x14);
      local_10 = (float)((param_1[0x55] - (uVar3 >> 1)) - 10);
      local_c = (float)param_1[0x56];
      fStack_2c = 0.0;
      local_8 = (float)(uint)local_30;
      local_30 = (float)FUN_0047a540(param_1[0x81]);
      fStack_2c = 0.0;
      local_4 = (float)(uint)local_30;
      FUN_0046b8b0(local_10,local_c,local_8,local_4,local_1c,local_18,local_14,local_28,local_24,
                   local_20,local_3c,fStack_38,local_34,param_2 & 0x20);
      FUN_0047a180(param_1[0x81],param_1[0x55] - (uVar3 >> 1),param_1[0x56],param_1 + 0x35);
      if (param_3 < 10) {
        iVar2 = iVar2 + 0x54;
        uVar3 = FUN_00401c50(DAT_00555b48,iVar2);
        local_30 = (float)(uVar3 + 0x14);
        local_10 = (float)((DAT_00555b9c - (uVar3 >> 1)) - 10);
        local_c = (float)DAT_00555ba0;
        fStack_2c = 0.0;
        local_8 = (float)(uint)local_30;
        local_30 = (float)FUN_0047a540(DAT_00555b48);
        fStack_2c = 0.0;
        local_4 = (float)(uint)local_30;
        FUN_0046b8b0(local_10,local_c,local_8,local_4,local_1c,local_18,local_14,local_28,local_24,
                     local_20,local_3c,fStack_38,local_34,0);
        FUN_0047a180(DAT_00555b48,DAT_00555b9c - (uVar3 >> 1),DAT_00555ba0,iVar2);
      }
    }
  }
  return;
}


// ==== RewardZoneCheck @ 00461dd0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 RewardZoneCheck(undefined4 *param_1,undefined4 *param_2)

{
  bool bVar1;
  int iVar2;
  int iVar3;
  undefined4 uVar4;
  
  bVar1 = false;
  iVar2 = FUN_00431560();
  if (iVar2 == 0) {
    return 0;
  }
  iVar3 = CheckpointBlobGet(*(undefined4 *)(iVar2 + 0x10));
  if (iVar3 == 0) {
    return 0;
  }
  if (*(int *)(iVar2 + 0x27c) == 1) {
    iVar2 = DAT_00555b68;
    if ((((_DAT_00555b60 < (float)DAT_0076ab24) &&
         ((float)DAT_0076ab24 < (float)*(uint *)(DAT_00555b68 + 4) + _DAT_00555b60)) &&
        (_DAT_00555b64 < (float)DAT_0076ab28)) &&
       ((float)DAT_0076ab28 < (float)*(uint *)(DAT_00555b68 + 8) + _DAT_00555b64)) {
      FUN_0046b770(0x24);
      if (((DAT_0076ab3c != 0) && (iVar2 = DAT_00555b70, DAT_00555b70 != 0)) ||
         (iVar2 = DAT_00555b6c, DAT_00555b6c != 0)) {
        FUN_0047dcb0(iVar2,0,&DAT_00555b60,0);
      }
      iVar2 = FUN_00435550();
      if (iVar2 != 0) {
        bVar1 = true;
        FUN_00464fa0(0x13,0);
        FUN_00462030();
        *param_2 = 1;
        FUN_00465190(0x29);
        FUN_00464f60(0x29,1);
      }
      goto LAB_00461f21;
    }
  }
  else {
    iVar2 = DAT_00555b74;
    if (DAT_00555b74 == 0) goto LAB_00461f21;
  }
  FUN_0047dcb0(iVar2,0,&DAT_00555b60,0);
LAB_00461f21:
  if ((((float)DAT_0076ab24 <= _DAT_00555b78) ||
      ((float)*(uint *)(DAT_00555b80 + 4) + _DAT_00555b78 <= (float)DAT_0076ab24)) ||
     (((float)DAT_0076ab28 <= _DAT_00555b7c ||
      ((float)*(uint *)(DAT_00555b80 + 8) + _DAT_00555b7c <= (float)DAT_0076ab28)))) {
    uVar4 = FUN_0047dcb0(DAT_00555b80,0,&DAT_00555b78,0);
  }
  else {
    FUN_0046b770(0x25);
    if (((DAT_0076ab3c != 0) && (iVar2 = DAT_00555b88, DAT_00555b88 != 0)) ||
       (iVar2 = DAT_00555b84, DAT_00555b84 != 0)) {
      FUN_0047dcb0(iVar2,0,&DAT_00555b78,0);
    }
    iVar2 = FUN_00435550();
    uVar4 = 0;
    if (iVar2 != 0) {
      bVar1 = true;
      *param_1 = 3;
      FUN_00464fa0(0x13,0);
      uVar4 = CheckpointStep();
    }
  }
  if (!bVar1) {
    iVar2 = FUN_00435550();
    uVar4 = 0;
    if (iVar2 != 0) {
      uVar4 = FUN_00464fa0(0x14,0);
    }
  }
  return uVar4;
}


// ==== OsfAdd @ 00459190 ====

void OsfAdd(int *param_1)

{
  int *piVar1;
  
  if ((*param_1 == 2) && (((param_1[0xfc] != 0 || (param_1[0xcc] != 0)) || (param_1[2] != 0)))) {
    if (DAT_004ffe54 == DAT_004ffe58) {
      DAT_004ffe58 = DAT_004ffe58 + 10;
      DAT_004ffe50 = FUN_0048dc80(DAT_004ffe50,DAT_004ffe58 * 0x14);
    }
    piVar1 = (int *)(DAT_004ffe50 + DAT_004ffe54 * 0x14);
    DAT_004ffe54 = DAT_004ffe54 + 1;
    *piVar1 = param_1[0xfc];
    piVar1[1] = param_1[0xcc];
    piVar1[2] = 0;
    piVar1[3] = 0;
    piVar1[4] = 0;
    if ((char *)param_1[2] != (char *)0x0) {
      _strncpy((char *)(piVar1 + 2),(char *)param_1[2],0xb);
    }
  }
  return;
}


// ==== OsfSnapshot @ 004592e0 ====

void OsfSnapshot(void)

{
  uint uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  
  if ((DAT_004ffe54 != 0) && (DAT_004ffe50 != (undefined4 *)0x0)) {
    if (DAT_004ffe5c != (undefined4 *)0x0) {
      FUN_0048db80(DAT_004ffe5c);
    }
    DAT_004ffe5c = (undefined4 *)FUN_0048dbd0(DAT_004ffe54 * 0x14);
    puVar3 = DAT_004ffe50;
    puVar4 = DAT_004ffe5c;
    for (uVar1 = DAT_004ffe54 * 5 & 0x3fffffff; uVar1 != 0; uVar1 = uVar1 - 1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
      *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
      puVar3 = (undefined4 *)((int)puVar3 + 1);
      puVar4 = (undefined4 *)((int)puVar4 + 1);
    }
    DAT_004ffe60 = DAT_004ffe54;
    DAT_004ffe64 = 1;
  }
  return;
}


// ==== OsfWrite @ 004593e0 ====

undefined4 OsfWrite(undefined4 param_1)

{
  int iVar1;
  undefined4 local_8;
  undefined4 local_4;
  
  iVar1 = FUN_0047f6e0(param_1,&DAT_004a2794);
  if (iVar1 != 0) {
    local_8 = 0x534f5252;
    local_4 = DAT_004ffe54;
    FUN_0047fa50(&local_8,4,2,iVar1);
    FUN_0047fa50(DAT_004ffe50,0x14,DAT_004ffe54,iVar1);
    FUN_0047fac0(iVar1);
    OsfSnapshot();
    DAT_004ffe64 = 0;
    return 1;
  }
  return 0;
}


// ==== ObjectiveTick @ 00458be0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void ObjectiveTick(undefined4 param_1,int param_2,undefined4 param_3,float param_4)

{
  bool bVar1;
  bool bVar2;
  int iVar3;
  undefined4 uVar4;
  float10 fVar5;
  int *piVar6;
  int local_10;
  int local_c [3];
  
  bVar1 = false;
  bVar2 = false;
  uVar4 = 0;
  local_c[0] = 0;
  if (3 < DAT_00505f88) {
    FUN_004588e0();
  }
  if ((_DAT_004ffbc0 & 1) == 0) {
    if ((_DAT_004ffbc0 & 2) == 0) {
      if ((_DAT_004ffbc0 & 4) == 0) {
        iVar3 = FUN_00459050(param_2,local_c + 2,param_3);
        if (iVar3 == 0) {
          FUN_00462800(param_3);
        }
        else if (local_c[2] == 0) {
          FUN_00458670(2);
        }
        else {
          FUN_00458670(1);
        }
      }
      else {
        iVar3 = FUN_0046a4c0(0x40b);
        if (iVar3 != 0) {
          local_10 = 2;
          FUN_00459360();
          if ((_DAT_004ffbc0 & 0x100) != 0) {
            local_10 = 3;
          }
          TransitionControl(0,1);
          bVar2 = true;
          if ((_DAT_004ffbc0 & 8) != 0) {
            if ((_DAT_004ffbc0 & 0x80) != 0) {
              FUN_004018c0(0,1);
            }
            _DAT_004ffbc0 = _DAT_004ffbc0 & 0xfffffff7;
          }
          FUN_0046ac60(0x16,0,0);
          uVar4 = 1;
          bVar1 = true;
        }
      }
    }
    else {
      iVar3 = FUN_0046a4c0(0x40b);
      if (iVar3 != 0) {
        bVar1 = true;
        TransitionControl(0,1);
        OsfSnapshot();
        if (*(int *)(param_2 + 0xf0) == 0) {
          if (*(int *)(param_2 + 0xec) == 0) {
            iVar3 = param_2 + 0xe4;
          }
          else {
            iVar3 = 0;
          }
          FUN_0042efc0(*(undefined4 *)(param_2 + 0xe0),iVar3);
          *(undefined4 *)(param_2 + 0xf0) = 1;
        }
        local_10 = 1;
        bVar2 = true;
        FUN_0046ac60(0x16,0,0);
        if ((_DAT_004ffbc0 & 8) != 0) {
          if ((_DAT_004ffbc0 & 0x20) != 0) {
            FUN_004018c0(0,1);
          }
          _DAT_004ffbc0 = _DAT_004ffbc0 & 0xfffffff7;
        }
        uVar4 = 1;
      }
    }
  }
  else {
    uVar4 = 1;
    TransitionControl(0,1);
    local_10 = 0;
    bVar2 = true;
    if ((_DAT_004ffbc0 & 8) != 0) {
      if ((_DAT_004ffbc0 & 0x10) != 0) {
        FUN_004018c0(0,1);
      }
      _DAT_004ffbc0 = _DAT_004ffbc0 & 0xfffffff7;
    }
    local_c[0] = 1;
    FUN_0046ac60(0x16,0,0);
  }
  if ((DAT_004ffe3c != -1) &&
     (_DAT_004ffe40 = _DAT_004ffe40 - param_4, _DAT_004ffe40 <= _DAT_0049e5a0)) {
    FUN_00401b20();
    DAT_004ffe3c = -1;
  }
  if (((bVar1) && ((DAT_00556ed0 & 0x800) == 0)) && (((byte)DAT_00556ecc & 8) != 0)) {
    DAT_00556ed0 = DAT_00556ed0 | 0x800;
    FUN_004297b0(0);
  }
  if ((DAT_004ffe44 != 0) && (DAT_004ffe38 != 0)) {
    iVar3 = FUN_00464c70(DAT_004ffe38,&param_4);
    if (iVar3 != 0) {
      FUN_004652b0();
      FUN_004652d0(0,0);
      DAT_004ffe3c = FUN_00464fa0(param_4,0);
      fVar5 = (float10)FUN_00465160(param_4);
      _DAT_004ffe40 = (float)((fVar5 - (float10)_DAT_0049e5a4) * (float10)_DAT_0049e59c);
      FUN_004652c0(1);
    }
    FUN_0048db80(DAT_004ffe38);
    DAT_004ffe38 = 0;
  }
  DAT_004ffe44 = uVar4;
  if (local_c[0] != 0) {
    FUN_004652c0(1);
  }
  if (bVar2) {
    if ((uint)(&DAT_004ffdd8)[local_10] < (uint)(&DAT_004ffdf8)[local_10]) {
      piVar6 = local_c;
      local_c[0] = 0x43eb0000;
      local_c[1] = 0x439d8000;
    }
    else {
      piVar6 = (int *)0x0;
    }
    FUN_004262e0(piVar6);
    if ((&DAT_004ffdd8)[local_10] == 0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = local_c;
      local_c[0] = 0x43020000;
      local_c[1] = 0x439d8000;
    }
    FUN_00426320(piVar6);
    if ((&DAT_004ffdd8)[local_10] != (&DAT_004ffde8)[local_10]) {
      FUN_004859b0((&DAT_004ffe08)[local_10]);
      FUN_00485210((&DAT_004ffe08)[local_10],(&DAT_004ffdd8)[local_10],&DAT_004a1d60,
                   (&DAT_004ffc48)[local_10]);
      (&DAT_004ffde8)[local_10] = (&DAT_004ffdd8)[local_10];
    }
    if (*(int *)(param_2 + 0xbc) != 0) {
      FUN_0047dcb0(*(int *)(param_2 + 0xbc),0,param_2 + 0xc0,0);
    }
    iVar3 = DAT_004ffe1c;
    if ((*(int *)(param_2 + 0x27c) != 1) && (iVar3 = DAT_004ffe20, *(int *)(param_2 + 0x27c) != 2))
    {
      iVar3 = DAT_004ffe18;
    }
    if (iVar3 != 0) {
      FUN_00485390(iVar3,0,param_3,0);
    }
    if ((&DAT_004ffe08)[local_10] != 0) {
      FUN_00485390((&DAT_004ffe08)[local_10],0,param_3,0);
    }
    if (DAT_004ffe24 != 0) {
      FUN_00485390(DAT_004ffe24,0,param_3,0);
    }
  }
  return;
}


// ==== RewardInit @ 0045f7f0 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 RewardInit(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  undefined *puVar3;
  float10 fVar4;
  undefined4 local_190;
  undefined4 local_18c;
  
  FUN_00431560();
  DAT_00555b54 = param_1;
  DAT_00555b58 = param_2;
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_Wallpaper_004a9418,0,0,0);
  uVar1 = FUN_004790b0(param_1,uVar1);
  DAT_00552998 = FUN_0047d9d0(uVar1);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_Display_004a9410,0);
  DAT_00552980 = FUN_004790d0(param_1,uVar1);
  if (DAT_00552980 == 2) {
    DAT_00552980 = 0;
  }
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_CentreText_004a9404,0);
  _DAT_00552984 = FUN_004790d0(param_1,uVar1);
  if (_DAT_00552984 == 2) {
    _DAT_00552984 = 0;
  }
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_ScrollSpeed_004a93f8,0);
  iVar2 = FUN_004790b0(param_1,uVar1);
  if (iVar2 == 0) {
    puVar3 = &DAT_004b8a14;
  }
  else {
    uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_ScrollSpeed_004a93f8,0);
    puVar3 = (undefined *)FUN_004790b0(param_1,uVar1);
  }
  fVar4 = (float10)FUN_0048d980(puVar3);
  DAT_00555b28 = (float)fVar4;
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_VertSpacing_004a93ec,0);
  iVar2 = FUN_004790b0(param_1,uVar1);
  if (iVar2 == 0) {
    puVar3 = &DAT_004b8a14;
  }
  else {
    uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_VertSpacing_004a93ec,0);
    puVar3 = (undefined *)FUN_004790b0(param_1,uVar1);
  }
  fVar4 = (float10)FUN_0048d980(puVar3);
  DAT_00555b2c = (float)fVar4;
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,&DAT_004a93e4,0);
  DAT_00555b3c = FUN_00479050(param_1,uVar1);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_TitleFont_004a93d8,0);
  DAT_00555b44 = FUN_00479050(param_1,uVar1);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_BackFont_004a93cc,0);
  DAT_00555b4c = FUN_00479050(param_1,uVar1);
  DAT_00555b40 = FUN_00479ee0(DAT_00555b3c);
  DAT_00555b48 = FUN_00479ee0(DAT_00555b44);
  DAT_00555b50 = FUN_00479ee0(DAT_00555b4c);
  FUN_0045faf0(param_1,param_2);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_CompleteText_004a93bc,0);
  DAT_00555b90 = FUN_00479050(param_1,uVar1);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_FailedText_004a93b0,0);
  DAT_00555b94 = FUN_00479050(param_1,uVar1);
  uVar1 = FUN_00478f50(param_2,s_Reward_004a89f8,s_QuitText_004a93a4,0);
  DAT_00555b98 = FUN_00479050(param_1,uVar1);
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_TextPos_004a939c,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 != 0) {
    FUN_00477440(iVar2,&local_190,&DAT_004a2378);
    DAT_00555b9c = FUN_0048db70(local_190);
    DAT_00555ba0 = FUN_0048db70(local_18c);
  }
  return 1;
}


// ==== TaskBlockInit @ 0045f030 ====

undefined4 TaskBlockInit(void)

{
  int iVar1;
  uint uVar2;
  undefined4 *puVar3;
  char **ppcVar4;
  char *local_28;
  undefined **local_24;
  char *local_20;
  char *local_1c;
  char *local_18;
  char *local_14;
  char *local_10;
  char *local_c;
  char *local_8;
  char *local_4;
  
  local_28 = s_Crystals_004a8a08;
  local_24 = &PTR_DAT_004a1e98;
  local_20 = s_Diggable_004a9364;
  local_1c = s_Constructions_004a9354;
  local_18 = s_Caverns_004a934c;
  local_14 = s_Figures_004a9344;
  local_10 = s_RockMonsters_004a9334;
  local_c = s_Oxygen_004a932c;
  local_8 = s_Timer_004a9324;
  local_4 = s_Score_004a931c;
  DAT_00552990 = (undefined4 *)FUN_0048dbd0(0x318c);
  puVar3 = DAT_00552990;
  for (iVar1 = 0xc63; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  uVar2 = 0;
  ppcVar4 = &local_28;
  DAT_00552990[0x73a] = DAT_00552990 + 0x73b;
  DAT_00552990[0x739] = 10;
  do {
    FUN_0048d9e0(DAT_00552990[0x73a] + 4 + uVar2,&DAT_004a1d60,*ppcVar4);
    uVar2 = uVar2 + 0x210;
    ppcVar4 = ppcVar4 + 1;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f3e0(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f530(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f5f0(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f230(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f1f0(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  uVar2 = 0;
  do {
    FUN_0045f290(DAT_00552990[0x73a] + uVar2);
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  return 1;
}


// ==== TaskBlockFinish @ 0045f750 ====

void TaskBlockFinish(void)

{
  byte *pbVar1;
  uint uVar2;
  
  uVar2 = 0;
  do {
    if ((*(byte *)(*(int *)(DAT_00552990 + 0x1ce8) + uVar2) & 2) != 0) {
      FUN_0045f7b0(*(int *)(DAT_00552990 + 0x1ce8) + uVar2);
    }
    pbVar1 = (byte *)(*(int *)(DAT_00552990 + 0x1ce8) + uVar2);
    if ((*pbVar1 & 1) != 0) {
      FUN_0045f7d0(pbVar1);
    }
    uVar2 = uVar2 + 0x210;
  } while (uVar2 < 0x14a0);
  FUN_0048db80(DAT_00552990);
  return;
}


// ==== TaskSoundsLoad @ 0045f1f0 ====

void TaskSoundsLoad(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Sounds_004a9370,param_1 + 4,0);
  uVar1 = FUN_00479050(DAT_00555b54,uVar1);
  *(undefined4 *)(param_1 + 0x208) = uVar1;
  return;
}


// ==== TaskFontLoad @ 0045f230 ====

void TaskFontLoad(int param_1)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Fonts_004a9378,param_1 + 4,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 != 0) {
    iVar2 = FUN_00479ee0(iVar2);
    *(int *)(param_1 + 0x204) = iVar2;
    if (iVar2 != 0) {
      return;
    }
  }
  *(undefined4 *)(param_1 + 0x204) = DAT_00555b40;
  return;
}


// ==== TaskBoxImages @ 0045f290 ====

void TaskBoxImages(uint *param_1)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined1 local_250 [64];
  undefined1 local_210 [128];
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_BoxImages_004a9388,param_1 + 1,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 == 0) {
    if ((DAT_00505f84 & 0x2000000) == 0) {
      FUN_0048d9e0(local_250,&PTR_DAT_004a13e8,param_1 + 1);
      uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Images_004a9380,local_250,0);
      iVar2 = FUN_00479050(DAT_00555b54,uVar1);
    }
    if (iVar2 == 0) {
      return;
    }
  }
  FUN_00477440(iVar2,&local_190,&DAT_004a2378);
  FUN_0048d9e0(local_210,&DAT_004a1d60,local_190);
  uVar3 = FUN_0048db70(local_18c);
  param_1[0x59] = uVar3;
  uVar3 = FUN_0048db70(local_188);
  param_1[0x5a] = uVar3;
  uVar3 = FUN_0047d9d0(local_210,0,0);
  param_1[0x83] = uVar3;
  if (uVar3 != 0) {
    FUN_0047dbf0(uVar3,0,0,0,0,0,0);
    *param_1 = *param_1 | 0x40;
    FUN_0048db80(iVar2);
  }
  return;
}


// ==== TaskImages @ 0045f3e0 ====

void TaskImages(uint *param_1)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined1 local_250 [64];
  undefined1 local_210 [128];
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Images_004a9380,param_1 + 1,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 == 0) {
    if ((DAT_00505f84 & 0x2000000) == 0) {
      FUN_0048d9e0(local_250,&PTR_DAT_004a13e8,param_1 + 1);
      uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Images_004a9380,local_250,0);
      iVar2 = FUN_00479050(DAT_00555b54,uVar1);
    }
    if (iVar2 == 0) {
      return;
    }
  }
  FUN_00477440(iVar2,&local_190,&DAT_004a2378);
  FUN_0048d9e0(local_210,&DAT_004a1d60,local_190);
  uVar3 = FUN_0048db70(local_18c);
  param_1[0x57] = uVar3;
  uVar3 = FUN_0048db70(local_188);
  param_1[0x58] = uVar3;
  uVar3 = FUN_0047d9d0(local_210,0,0);
  param_1[0x80] = uVar3;
  if (uVar3 != 0) {
    FUN_0047dbf0(uVar3,0,0,0,0,0,0);
    *param_1 = *param_1 | 2;
    FUN_0048db80(iVar2);
  }
  return;
}


// ==== TaskBoxRect @ 0045f530 ====

void TaskBoxRect(uint *param_1)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,&DAT_004a26b0,param_1 + 1,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 != 0) {
    FUN_00477440(iVar2,&local_190,&DAT_004a2378);
    FUN_0048d9e0(param_1 + 0x15,&DAT_004a1d60,local_190);
    FUN_00413f00(param_1 + 0x15,0x5f,0x20);
    uVar3 = FUN_0048db70(local_18c);
    param_1[0x55] = uVar3;
    uVar3 = FUN_0048db70(local_188);
    param_1[0x56] = uVar3;
    *param_1 = *param_1 | 0x31;
    FUN_0048db80(iVar2);
  }
  return;
}


// ==== TaskFlicPath @ 0045f5f0 ====

void TaskFlicPath(uint *param_1)

{
  undefined4 uVar1;
  int iVar2;
  uint uVar3;
  undefined1 local_250 [64];
  undefined1 local_210 [128];
  undefined4 local_190;
  undefined4 local_18c;
  undefined4 local_188;
  undefined4 local_184;
  undefined4 local_180;
  
  uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Flics_004a9394,param_1 + 1,0);
  iVar2 = FUN_00479050(DAT_00555b54,uVar1);
  if (iVar2 == 0) {
    if ((DAT_00505f84 & 0x1000000) == 0) {
      FUN_0048d9e0(local_250,&PTR_DAT_004a13e8,param_1 + 1);
      uVar1 = FUN_00478f50(DAT_00555b58,s_Reward_004a89f8,s_Flics_004a9394,local_250,0);
      iVar2 = FUN_00479050(DAT_00555b54,uVar1);
    }
    if (iVar2 == 0) {
      return;
    }
  }
  FUN_00477440(iVar2,&local_190,&DAT_004a2378);
  FUN_0048d9e0(local_210,&DAT_004a1d60,local_190);
  FUN_0048d9e0(param_1 + 0x5b,&DAT_004a1d60,local_210);
  uVar3 = FUN_0048db70(local_18c);
  param_1[0x7d] = uVar3;
  uVar3 = FUN_0048db70(local_188);
  param_1[0x7e] = uVar3;
  uVar3 = FUN_0048db70(local_184);
  param_1[0x7b] = uVar3;
  uVar3 = FUN_0048db70(local_180);
  param_1[0x7c] = uVar3;
  *param_1 = *param_1 | 4;
  FUN_0048db80(iVar2);
  return;
}


// ==== TaskImportanceRefresh @ 004600a0 ====

void TaskImportanceRefresh(int param_1)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  float10 fVar4;
  
  puVar1 = (undefined4 *)FUN_00431560();
  uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Importance_004a9490,param_1 + 4,0);
  iVar3 = FUN_004790b0(DAT_00555b54,uVar2);
  if (iVar3 != 0) {
    uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Importance_004a9490,param_1 + 4,0)
    ;
    uVar2 = FUN_004790b0(DAT_00555b54,uVar2);
    fVar4 = (float10)FUN_0048d980(uVar2);
    *(float *)(param_1 + 0x24) = (float)fVar4;
    return;
  }
  fVar4 = (float10)FUN_0048d980(&DAT_004b8a14);
  *(float *)(param_1 + 0x24) = (float)fVar4;
  return;
}


// ==== TaskQuotaRefresh @ 00460140 ====

void TaskQuotaRefresh(int param_1)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  int iVar3;
  
  puVar1 = (undefined4 *)FUN_00431560();
  uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Quota_004a8a00,param_1 + 4,0);
  iVar3 = FUN_004790b0(DAT_00555b54,uVar2);
  if (iVar3 != 0) {
    uVar2 = FUN_00478f50(DAT_00555b58,*puVar1,s_Reward_004a89f8,s_Quota_004a8a00,param_1 + 4,0);
    uVar2 = FUN_004790b0(DAT_00555b54,uVar2);
    uVar2 = FUN_0048db70(uVar2);
    *(undefined4 *)(param_1 + 0x28) = uVar2;
    return;
  }
  uVar2 = FUN_0048db70(&DAT_004b8a14);
  *(undefined4 *)(param_1 + 0x28) = uVar2;
  return;
}


// ==== TaskFlushImages @ 0045f7b0 ====

void TaskFlushImages(int param_1)

{
  FUN_0047d470(*(undefined4 *)(param_1 + 0x200));
  return;
}


// ==== TaskFlushFont @ 0045f7d0 ====

void TaskFlushFont(int param_1)

{
  FUN_0047a550(*(undefined4 *)(param_1 + 0x204));
  return;
}


// ==== CrystalQuotaSetup @ 0045c040 ====

void CrystalQuotaSetup(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 uVar1;
  int iVar2;
  undefined *puVar3;
  
  uVar1 = FUN_00478f50(param_2,param_3,s_Reward_004a89f8,s_Quota_004a8a00,s_Crystals_004a8a08,0);
  iVar2 = FUN_004790b0(param_1,uVar1);
  if (iVar2 == 0) {
    puVar3 = &DAT_004b8a14;
  }
  else {
    uVar1 = FUN_00478f50(param_2,param_3,s_Reward_004a89f8,s_Quota_004a8a00,s_Crystals_004a8a08,0);
    puVar3 = (undefined *)FUN_004790b0(param_1,uVar1);
  }
  DAT_005007ec = FUN_0048db70(puVar3);
  if ((0x1a < DAT_005007ec) || (DAT_005007ec == 0)) {
    DAT_005007ec = 0x1a;
  }
  return;
}


// ==== RewardScoreAdd @ 00462800 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void RewardScoreAdd(float param_1)

{
  DAT_0055573c = DAT_0055573c - _DAT_0049e6bc;
  _DAT_00555738 = param_1 + _DAT_00555738;
  return;
}


