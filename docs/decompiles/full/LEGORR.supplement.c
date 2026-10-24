/* Legacy named decompilation supplement.
 * These 11 entries occur in engine-region.c but not at matching entry
 * addresses in the full Ghidra manifest. Kept verbatim for lookup; they are
 * not included in full-corpus coverage counts. See README.md.
 */

// ==== SavObjCallbackB @ 00417700 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void SavObjCallbackB(float param_1,undefined4 param_2)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  char *pcVar4;

  piVar1 = DAT_00557598;
  piVar2 = (int *)RegistryValueLookupWithLock(DAT_00556ffc,param_2);
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
    SelectionRecordPointSet(DAT_005560e4,DAT_00556148,DAT_0055614c);
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
  TerrainCellFlagCleanup(DAT_005560d4,fVar10);
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
  TerrainDirtyCellUpdate(*(undefined4 *)(DAT_005560d4 + 4),fVar10);
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
    GraphicsOverlayVisibilitySet();
    FUN_00463230();
    FUN_0043b850(DAT_00556160,*(undefined4 *)(DAT_005560ec + 0x24),DAT_00556400,DAT_00556404,1);
    uVar4 = RecordField14Get(DAT_005560ec,1);
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
    uVar4 = RecordField14Get(DAT_005560ec,0);
    FUN_0043b820(DAT_00556160,uVar4);
    FUN_0043b850(DAT_00556160,0,0,0,0);
    IndexedSurfaceEntriesRestore();
    GraphicsOverlayVisibilitySet();
    FUN_00434530();
  }
  FUN_00473b40(DAT_00556100,1);
  iVar3 = RequestRecordSetFlag2ClearFlag8();
  if (iVar3 == 0) {
    DAT_00556ecc = DAT_00556ecc & 0xfffffffd;
  }
  else {
    DAT_00556ecc = DAT_00556ecc | 2;
  }
  if (DAT_00556ec8 == 1) {
    FUN_00424830();
    ApplyDefaultTransformToFlaggedRenderObjects();
    ScreenSpaceEffectDispatchIfEnabled();
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
    TerrainCellChangeQueueFlush();
  }
  FUN_00477fa0();
  TransientEffectRegistryEnableSet();
  FUN_004651a0();
  if ((DAT_00556ec8 == 1) && ((DAT_00556ecc & 0x100000) == 0)) {
    FUN_00435300(DAT_005560d4,DAT_005560dc,fVar10,param_1);
    FUN_00454600(DAT_005560d4,DAT_005560dc,fVar10,param_1);
  }
  TerrainCellChangeQueueFlush();
  if (((DAT_00556ecc & 2) == 0) || ((DAT_00556ecc & 0x1000) == 0)) {
    iVar3 = 0;
  }
  else {
    iVar3 = 1;
  }
  if (iVar3 != 0) {
    UpdateRenderViewportState();
  }
  PeriodicTimer25Advance(fVar10,iVar3);
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
    PositionTrackerUpdateAll();
    FUN_0045a730(0xb,param_1);
    FUN_0040e7f0();
    if ((DAT_00556ed0 & 0x1000) == 0) {
      FUN_0041b490();
    }
    TimedStatusAndEventTick();
    FUN_0045a730(5,param_1);
    fVar9 = param_1;
    fVar1 = param_1;
    if (DAT_005560d4 != 0) {
      fVar9 = *(float *)(DAT_005560d4 + 0x8c);
      fVar1 = (float)(*(int *)(DAT_005560d4 + 0x8c) - *(int *)(DAT_005560d4 + 0x98));
    }
    FUN_0045c0d0(fVar9,fVar1,fVar10);
    UiOverlayAnimationApply();
    FUN_0045a460(5,DAT_00556128,0x10,0x1d5,1,&DAT_004a2b54,
                 *(int *)(DAT_005560d4 + 0xa0) + *(int *)(DAT_005560d4 + 0xac) * 5);
    FUN_0045a730(7,param_1);
    FUN_00463d20();
    FUN_0041a080();
    FUN_0045a730(10,param_1);
    FUN_00419f30();
  }
  FUN_00418a00();
  GlobalOptionLowBitsEnabled();
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
    iVar3 = GridRecordCountDifferenceGet(0xf,0,0,1);
    if (iVar3 == 0) {
      uVar11 = 0;
      iVar12 = 0;
      iVar3 = FUN_00469f40(2,0);
      if (iVar3 != 0) {
        do {
          iVar3 = GridRecordCountDifferenceGet(2,0,uVar11,1);
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
    iVar3 = GlobalOptionLowBitsEnabled();
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
        iVar3 = GlobalOptionLowBitsEnabled();
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
    GridCellReadSurfaceVector(*(undefined4 *)(DAT_005560d4 + 4),DAT_00556180,DAT_00556184,&fStack_c);
    RenderInterfaceTransformPoint(DAT_005560dc,&fStack_28,&fStack_c);
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
      iVar12 = ConfiguredVariantPropertyRead();
      if (iVar3 != iVar12) {
        ConfiguredVariantPropertySet(uStack_30,iVar3);
      }
    }
  }
  FUN_0045c4a0();
  _DAT_00556efc = _DAT_00556efc - fStack_2c;
  return 1;
}
// ==== EngineShutdownAlt @ 00424cf0 ====

void EngineShutdownAlt(void)

{
  FUN_004250a0();
  return;
}
// ==== EngineShutdown @ 00424d00 ====

void EngineShutdown(void)

{
  char *pcVar1;
  uint uVar2;
  int iVar3;

  GlobalResourceHandleRelease();
  FUN_0045f750();
  FUN_004297b0(0);
  FUN_00429810(0);
  FUN_0042f0c0();
  ResourceHandleSetRelease();
  ReleaseLinkedObjectList(DAT_005560c0);
  RenderResourceTrackAndRetain(DAT_005560dc);
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
      RecordResourceRelease(DAT_005561e4 + iVar3);
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
  RockFallStyleSceneResourcesRelease();
  HandleArrayReleaseAndReset();
  EntityPoolResourcesRelease();
  FUN_00472800();
  FUN_00476d80();
  if (DAT_00505f88 == 10) {
    iVar3 = CdAudioPlaybackAvailable();
    pcVar1 = s_LoaderProfile_txt_004a3acc;
    if (iVar3 == 0) {
      pcVar1 = s_LoaderProfileNoSound_txt_004a3ab0;
    }
  }
  else {
    pcVar1 = (char *)0x0;
  }
  EngineLoaderProfileEntriesRelease(pcVar1);
  FUN_00484c90();
  FUN_0047d430();
  FUN_00487760();
  DirectSoundObjectsShutdown();
  ProjectionOverlayDrawSubmit(0,0);
  return;
}
// ==== EntityFlagWordBehaviorEvaluate @ 0043c810 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 EntityFlagWordBehaviorEvaluate(int *param_1,float *param_2)

{
  uint *puVar1;
  float fVar2;
  int *piVar3;
  float fVar4;
  float fVar5;
  uint uVar6;
  bool bVar7;
  int *piVar8;
  short sVar9;
  int iVar10;
  int iVar11;
  uint uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  float fVar15;
  uint uVar16;
  float10 fVar17;
  undefined4 uStack_34;
  float fStack_30;
  float fStack_2c;
  undefined4 uStack_28;
  float fStack_24;
  float fStack_20;
  int iStack_1c;
  undefined4 uStack_18;
  int iStack_14;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  float fStack_4;

  piVar8 = param_1;
  fVar2 = *param_2;
  bVar7 = true;
  _DAT_00556090 = param_1;
  FUN_0043c6b0(param_1,fVar2);
  if ((piVar8[0xfb] & 0x200000U) != 0) {
    piVar8[0xba] = 0;
    SceneActivityUpdateDispatch(piVar8);
    piVar8[0xfb] = piVar8[0xfb] & 0xffdfffff;
  }
  if ((piVar8[0xf9] & 0x8000000U) != 0) {
    if ((float)piVar8[0xda] <= _DAT_0049e3a4) {
      AttachedEntityWake(piVar8);
    }
    piVar8[0xda] = (int)((float)piVar8[0xda] - fVar2);
    return 0;
  }
  if ((piVar8[0xfb] & 0x40000U) != 0) {
    iVar10 = EntityTypeComponentGet(piVar8);
    if ((iVar10 != 0) && (iVar11 = FUN_00469fc0(piVar8), iVar11 != 0)) {
      iVar10 = SoundSubmissionRouteWrapper(iVar10,iVar11,1,1,0);
      piVar8[0xd5] = iVar10;
      piVar8[0xfb] = piVar8[0xfb] | 0x80000;
    }
    piVar8[0xfb] = piVar8[0xfb] & 0xfffbffff;
  }
  if ((((byte)DAT_00556ed0 & 1) == 0) || (iVar10 = EntityCanDoOrRideCheck(piVar8), iVar10 == 0)) {
    FUN_00407510(piVar8,0);
    uVar12 = piVar8[0xfb] & 0xffffffdf;
  }
  else {
    FUN_004074b0(piVar8);
    uVar12 = piVar8[0xfb] | 0x20;
  }
  piVar8[0xfb] = uVar12;
  if ((((*piVar8 == 1) && ((piVar8[0xfb] & 0x40U) != 0)) && (piVar8[0xbf] == 0)) &&
     (piVar8[0xdb] == 0)) {
    piVar8[0xfb] = piVar8[0xfb] & 0xffffffbf;
    EntityLavaCrustActivityRegister(piVar8);
  }
  if ((*piVar8 == 2) || (*piVar8 == 1)) {
    iVar10 = CurrentLevelGet();
    EntityPositionToTerrainCell(piVar8,&fStack_20,&iStack_1c);
    if ((*(byte *)(*(int *)(iVar10 + 0xb8) + 0xc +
                  (*(int *)(iVar10 + 0x14) * iStack_1c + (int)fStack_20) * 0x48) & 0x80) != 0) {
      EmergeMapTriggerSpawn(iVar10,&fStack_20,0);
    }
  }
  BuildingDepositStateMonitor(piVar8);
  if (((*piVar8 == 2) && ((*(byte *)(piVar8 + 0xf9) & 8) != 0)) &&
     ((piVar3 = (int *)piVar8[0xdb], piVar3 != (int *)0x0 && (*piVar3 == 1)))) {
    fVar17 = (float10)SceneObjectAttachedResourceRelease(piVar3[3]);
    AnimationObjectTimeUpdate(piVar8[4],(float)fVar17);
  }
  else {
    fVar17 = (float10)EntityActivityTick(piVar8,fVar2);
    fStack_10 = (float)fVar17;
  }
  uVar12 = EntityStatsWordGet(piVar8);
  if ((uVar12 & 0x80000070) != 0) {
    EntitySpawnRequestQueueProcess(piVar8);
  }
  if ((*piVar8 != 4) || (iVar10 = EntitySharedEligibilityCheck(piVar8,0), iVar10 != 0)) {
    AirSupplyAmountUpdate(piVar8,fVar2);
  }
  if ((DAT_004dee14 & 4) == 0) {
    piVar8[0xfa] = piVar8[0xfa] & 0xbfffffff;
  }
  else {
    FUN_0043c570(piVar8);
  }
  uVar12 = piVar8[0xf8];
  if ((uVar12 & 0x400000) != 0) {
    if (piVar8[0xb9] == 0) {
      piVar8[0xfa] = piVar8[0xfa] | 0x800000;
      return 0;
    }
    uVar13 = EntityTypeComponentGet(piVar8);
    CameraTeleportOrOffset(uVar13,2,0,0,-fVar2);
    fVar17 = (float10)DDiSceneAdvanceByTime(piVar8[0xb9],fVar2);
    if (fVar17 <= (float10)_DAT_0049e3a4) {
      return 0;
    }
    piVar8[0xd0] = -0x40800000;
    piVar8[0xfa] = piVar8[0xfa] | 0x800000;
    piVar8[0xf8] = piVar8[0xf8] & 0xffbfffff;
    DDiSceneObjectDestroy(piVar8[0xb9]);
    return 0;
  }
  if ((uVar12 & 8) == 0) goto LAB_0043ce6f;
  if ((uVar12 & 0x10) != 0) {
    uVar13 = EntityTypeComponentGet(piVar8);
    uVar14 = NestedWordAtOffsetC0OrC4Get(piVar8,0,1,1,0);
    iVar10 = SoundSubmissionRouteWrapper(uVar13,uVar14);
    piVar8[0xd4] = iVar10;
    piVar8[0xfb] = piVar8[0xfb] | 0x100000;
    piVar8[0xf8] = piVar8[0xf8] & 0xffffffef;
  }
  iVar10 = piVar8[9];
  if (iVar10 == 0) {
LAB_0043cdea:
    piVar8[0xf8] = piVar8[0xf8] & 0xfffffff7;
    piVar8[0xfa] = piVar8[0xfa] & 0xfdffffff;
    uVar13 = EntityTypeComponentGet(piVar8);
    iVar10 = piVar8[0xd4];
LAB_0043ce1d:
    SoundHandleStopForward(iVar10);
    piVar8[0xfb] = piVar8[0xfb] & 0xffefffff;
    uVar14 = NestedWordAtOffsetC0OrC4Get(piVar8,1,0,1,0);
    SoundSubmissionRouteWrapper(uVar13,uVar14);
    FUN_00484c90(0,0,0);
    uVar12 = piVar8[0xfa] & 0xffffbfff;
LAB_0043ce69:
    piVar8[0xfa] = uVar12;
  }
  else {
    fVar4 = *(float *)(iVar10 + piVar8[0xb] * 0x14);
    fVar5 = *(float *)(iVar10 + 4 + piVar8[0xb] * 0x14);
    uVar13 = __ftol(piVar8[0xfa] & 0x2000000,1);
    uVar13 = __ftol(uVar13);
    iVar10 = TerrainCellWalkabilityQuery(piVar8,uVar13);
    if (iVar10 == 0) goto LAB_0043cdea;
    iVar10 = EntityWorkObjectPositionGet(piVar8,&fStack_30,&uStack_34);
    if (iVar10 == 0) goto LAB_0043ce6f;
    if (*piVar8 == 2) {
      uVar13 = FUN_00423080(0);
      uVar13 = RendererCachedHandle40GetOrCreate(piVar8[4],uVar13);
      FUN_00484c90(uVar13);
    }
    uVar13 = __ftol();
    uVar13 = __ftol(uVar13);
    uVar13 = TerrainCellMaterialGet(uVar13);
    fVar17 = (float10)TerrainTypeMovementSpeedGet(piVar8,uVar13);
    param_1 = (int *)(float)fVar17;
    uVar12 = EntityStatsWordGet(piVar8);
    if (((uVar12 & 1) != 0) && ((piVar8[0xfa] & 0x2000000U) != 0)) {
      param_1 = (int *)((float)param_1 * _DAT_0049e3b4);
    }
    uVar13 = __ftol(param_1,fVar2);
    uVar13 = __ftol(uVar13);
    iVar10 = TerrainCellProgressUpdate(uVar13);
    if (iVar10 == 0) {
      iVar10 = CurrentLevelTerrainGridPick(fStack_30,uStack_34,&fStack_24,&fStack_20);
      if (((iVar10 == 0) || (fStack_24 != fVar4)) || (fStack_20 != fVar5)) goto LAB_0043ce6f;
      uVar12 = piVar8[0xfa] | 0x4000;
      goto LAB_0043ce69;
    }
    uVar13 = __ftol(piVar8[0xfa] & 0x2000000,1);
    uVar13 = __ftol(uVar13);
    iVar10 = TerrainCellWalkabilityQuery(piVar8,uVar13);
    if (iVar10 != 0) {
      fVar15 = (float)__ftol();
      fStack_c = fVar15;
      fStack_8 = (float)__ftol();
      if ((piVar8[0xfa] & 0x2000000U) == 0) {
        uVar13 = CurrentLevelGet(fVar4,fVar5,0);
        iVar10 = TerrainCellMutation(uVar13);
      }
      else {
        uVar13 = CurrentLevelGet(fVar15,fStack_8);
        iVar10 = TerrainCellInitialize(uVar13);
      }
      if (iVar10 == 0) goto LAB_0043ce6f;
      EntityCommandUnlinkByKind(piVar8,8,0,1);
      piVar8[0xf8] = piVar8[0xf8] & 0xfffffff7;
      TerrainCellPredugEdgeWallFlagSet(&fStack_c,0);
      if (*(char *)(piVar8[9] + -3 + piVar8[10] * 0x14) == '\x01') {
        *(undefined1 *)(piVar8[9] + -3 + piVar8[10] * 0x14) = 0;
      }
      uVar13 = EntityTypeComponentGet(piVar8);
      iVar10 = piVar8[0xd4];
      goto LAB_0043ce1d;
    }
  }
LAB_0043ce6f:
  if ((piVar8[0xf9] & 0x4000U) != 0) {
    FUN_0044c1b0(piVar8,fVar2);
  }
  if ((((piVar8[0xf9] & 0x40000000U) != 0) && (_DAT_0049e3a4 < fVar2)) &&
     (iVar10 = EntityTypeComponentGet(piVar8), iVar10 != 0)) {
    FUN_00475500(iVar10,0,&fStack_20,&fStack_c);
    fStack_c = fStack_c * _DAT_0049e3d4;
    fStack_8 = fStack_8 * _DAT_0049e3d4;
    fStack_4 = fStack_4 * _DAT_0049e3d4;
    FUN_00479500(&fStack_30);
    fStack_8 = _DAT_004a48b0 * _DAT_0049e3f4 * (fStack_2c + fStack_8);
    fStack_c = _DAT_004a48b0 * _DAT_0049e3f4 * (fStack_30 + fStack_c);
    fStack_4 = -1.0;
    FUN_00475470(iVar10,0,fStack_20,iStack_1c,uStack_18,fStack_c,fStack_8,0xbf800000);
    _DAT_004a48b0 = _DAT_004a48b0 - fVar2 * _DAT_0049e3fc;
    if (_DAT_004a48b0 < _DAT_0049e3a4) {
      _DAT_004a48b0 = 1.0;
      piVar8[0xf9] = piVar8[0xf9] & 0xbfffffff;
    }
  }
  uVar12 = piVar8[0xf8];
  if ((uVar12 & 0x100000) != 0) {
    FUN_0044bfd0(piVar8);
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x4000000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf8] = uVar12 & 0xfbffffff;
    }
    goto LAB_0043ed80;
  }
  uVar16 = piVar8[0xfb];
  if ((uVar16 & 0x1000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xfb] = uVar16 & 0xffffefff | 0x2000;
    }
    goto LAB_0043ed80;
  }
  if ((uVar16 & 0x4000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xfb] = uVar16 & 0xffffbfff;
      EntityActivityRequestSet(piVar8,0,0);
    }
    goto LAB_0043ed80;
  }
  if ((uVar16 & 0x2000) != 0) {
    EntityActivityRequestSet(piVar8,0x37,0);
    goto LAB_0043ed80;
  }
  uVar6 = piVar8[0xf9];
  if ((uVar6 & 0x4000) != 0) goto LAB_0043ed80;
  if ((uVar12 & 4) != 0) {
    if ((uVar12 & 0x80) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        uVar13 = EntityTypeComponentGet(piVar8);
        FUN_00475580(uVar13,1,0,0x3f800000,0,0x3fc90fdb);
        DDiSceneFrameUpdateAndCallbacks(uVar13,0);
        piVar8[0xf8] = piVar8[0xf8] & 0xffffff7f;
      }
      goto LAB_0043ed80;
    }
    if ((float)piVar8[0xb5] <= _DAT_0049e3a4) goto LAB_0043ed80;
    uVar13 = EntityTypeComponentGet(piVar8);
    if ((DAT_004dece8 == piVar8[0xba]) || (DAT_004ded14 == piVar8[0xba])) {
      uVar14 = 0xbfc90fdb;
    }
    else {
      uVar14 = 0x3fc90fdb;
    }
    FUN_00475580(uVar13,1,0,0x3f800000,0,uVar14);
    uVar12 = piVar8[0xf8];
    piVar8[0xf8] = uVar12 & 0xfffffffb | 1;
    if (piVar8[10] != 0) {
      if (((uVar12 & 0x800000) != 0) && (*piVar8 == 2)) {
        EntityActivityRequestSet(piVar8,3,0);
        goto LAB_0043ed80;
      }
LAB_0043e472:
      EntityActivityRequestSet(piVar8,1,0);
      goto LAB_0043ed80;
    }
LAB_0043e9ee:
    EntityActivityRequestSet(piVar8,0,0);
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x20) != 0) {
    FUN_00449700(piVar8);
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x80000000) != 0) {
LAB_0043e851:
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf8] = uVar12 & 0x7fffffff;
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x10000000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      uVar13 = EntityTypeComponentGet(piVar8);
      piVar8[0xf8] = piVar8[0xf8] & 0xefffffff;
      FUN_00475470(uVar13,0,piVar8[0xb2],piVar8[0xb3],0,0,0,0xbf800000);
      EntityActivityRequestSet(piVar8,0,0);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x400000) != 0) {
    if ((uVar6 & 0x10000) != 0) {
      FUN_004420d0(piVar8,1);
    }
    if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
       (uVar12 = piVar8[0xf9], piVar8[0xf9] = uVar12 & 0xffbfffff, (uVar12 & 0x20000) == 0)) {
      FUN_004420d0(piVar8,1);
    }
    goto LAB_0043ed80;
  }
  if ((uVar16 & 0x8000) != 0) {
    if ((float)piVar8[0xb5] <= _DAT_0049e3a4) goto LAB_0043ed80;
    piVar8[0xfb] = uVar16 & 0xffff7fff;
    if (piVar8[0xbf] != 0) {
      iVar10 = EntityVisualResourceGetOrCreate(piVar8[0xbf]);
      if (iVar10 != 0) {
        DDiSceneMoveOrPose(iVar10,0,&fStack_c);
        FUN_00475500(iVar10,0,&fStack_20,0);
        uVar13 = FUN_00431570(fStack_c,fStack_8);
        fVar17 = (float10)FUN_0044f940(uVar13);
        uVar13 = EntityTypeComponentGet(piVar8,0,fStack_c,fStack_8,(float)fVar17);
        DDiSceneObjectPositionSet(uVar13);
        uVar13 = EntityTypeComponentGet(piVar8,0,fStack_20,iStack_1c,uStack_18,0,0,0xbf800000);
        FUN_00475470(uVar13);
      }
      EntityActivityToggle(piVar8[0xbf],0);
      EntityActivityRequestSet(piVar8,0,0);
      goto LAB_0043ed80;
    }
    goto LAB_0043e9ee;
  }
  if ((uVar6 & 0x80000000) != 0) {
    if (piVar8[0xbf] == 0) {
      piVar8[0xf9] = uVar6 & 0x7fffffff;
    }
    else {
      uVar13 = EntityTypeComponentGet(piVar8);
      uVar14 = EntityTypeComponentGet(piVar8[0xbf]);
      iVar10 = piVar8[0xbf];
      DDiSceneMoveOrPose(uVar14,0,&fStack_c);
      FUN_00475500(uVar14,0,&fStack_30,&fStack_20);
      DDiSceneObjectPositionSet(uVar13,0,fStack_c,fStack_8,fStack_4);
      FUN_00475470(uVar13,0,fStack_30,fStack_2c,uStack_28,fStack_20,iStack_1c,uStack_18);
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        *(int **)(iVar10 + 0x308 + *(int *)(iVar10 + 0x324) * 4) = piVar8;
        *(int *)(*(int *)(iVar10 + 0x308 + *(int *)(iVar10 + 0x324) * 4) + 0x304) = iVar10;
        *(int *)(*(int *)(iVar10 + 0x308 + *(int *)(iVar10 + 0x324) * 4) + 800) =
             *(int *)(iVar10 + 0x324);
        *(uint *)(iVar10 + 0x3e0) = *(uint *)(iVar10 + 0x3e0) | 0x400;
        EntityActivityToggle(iVar10,0);
        *(int *)(iVar10 + 0x324) = *(int *)(iVar10 + 0x324) + 1;
        piVar8[0xf9] = piVar8[0xf9] & 0x7fffffff;
        EntityActivityRequestSet(piVar8,0,0);
        EntityCommandUnlinkByKind(piVar8,0x1e,0,1);
        *(uint *)(iVar10 + 0x3ec) = *(uint *)(iVar10 + 0x3ec) & 0xfffeffff;
      }
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x10000000) != 0) {
    if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
       (piVar8[0xf9] = uVar6 & 0xefffffff, (uVar12 & 0x400) != 0)) {
      *(uint *)(piVar8[0xc2] + 1000) = *(uint *)(piVar8[0xc2] + 1000) & 0x7fffffff;
      uVar13 = EntityTypeComponentGet(piVar8[0xc2],0,&fStack_c);
      DDiSceneMoveOrPose(uVar13);
      SoundSubmissionRouteWrapper(0,0x1a,0,0,&fStack_c);
      RoverWorkQueueUpdate(piVar8[0xc2],1);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x800000) != 0) {
    if ((uVar6 & 0x10000) != 0) {
      FUN_004420d0(piVar8,2);
    }
    if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
       (uVar12 = piVar8[0xf9], piVar8[0xf9] = uVar12 & 0xff7fffff, (uVar12 & 0x20000) == 0)) {
      FUN_004420d0(piVar8,2);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x1000000) != 0) {
    if ((uVar6 & 0x10000) != 0) {
      FUN_004420d0(piVar8,3);
    }
    if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
       (uVar12 = piVar8[0xf9], piVar8[0xf9] = uVar12 & 0xfeffffff, (uVar12 & 0x20000) == 0)) {
      FUN_004420d0(piVar8,3);
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x800) != 0) {
    if ((*piVar8 == 3) && (iVar10 = RendererCachedHandle4cIsNotOne(piVar8[5]), iVar10 != 0)) {
      EntityClearLinkedObjectState(piVar8);
    }
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf8] = piVar8[0xf8] & 0xfffff7ff;
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x4000) != 0) {
    iVar10 = piVar8[0xbf];
    if ((((iVar10 != 0) && ((*(uint *)(iVar10 + 0x3e0) & 0x2000) == 0)) &&
        ((*(uint *)(iVar10 + 1000) & 0x1000000) == 0)) &&
       (*(int *)(iVar10 + 0x324) != *(int *)(iVar10 + 0x328))) {
      piVar8[0xf8] = uVar12 & 0xffffbfff | 0x2000;
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x2000) != 0) {
    if ((float)piVar8[0xb5] <= _DAT_0049e3a4) {
      if (piVar8[0xbf] != 0) {
        puVar1 = (uint *)(piVar8[0xbf] + 1000);
        *puVar1 = *puVar1 | 0x1000000;
      }
    }
    else {
      piVar8[0xf8] = uVar12 & 0xffffdfff;
      EntityCommandUnlinkByKind(piVar8,5,piVar8,1);
      FUN_0043eea0(piVar8);
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x20000000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf8] = uVar12 & 0xdfffffff;
      piVar8[0xfa] = piVar8[0xfa] & 0xffffbfff;
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x200000) != 0) {
    if ((uVar6 & 0x10000) != 0) {
      TerrainCellVariantUpdateQueue(piVar8);
    }
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf9] = piVar8[0xf9] & 0xffdfffff;
      EntityCommandUnlinkByKind(piVar8,0x19,0,1);
      if ((piVar8[0xf9] & 0x20000U) == 0) {
        TerrainCellVariantUpdateQueue(piVar8);
      }
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x40000) != 0) {
    if ((uVar6 & 0x10000) != 0) {
      FUN_004474f0(piVar8);
    }
    if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
       (uVar12 = piVar8[0xf9], piVar8[0xf9] = uVar12 & 0xfffbffff, (uVar12 & 0x20000) == 0)) {
      FUN_004474f0(piVar8);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x20000000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf9] = uVar6 & 0xdfffffff;
      piVar8[0xfa] = piVar8[0xfa] & 0x7fffffff;
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x1000000) != 0) {
    uVar12 = EntityStateFlagsGet(piVar8);
    if ((((uVar12 & 0x80000) != 0) && (iVar10 = piVar8[0xbf], iVar10 != 0)) &&
       ((*(byte *)(iVar10 + 0x3ec) & 0x10) == 0)) {
      iVar10 = EntitySharedEligibilityCheck(iVar10,0);
      if (iVar10 != 0) {
        MessageDispatch(0xf,0,piVar8[0xbf],0);
        FUN_00435aa0(1);
      }
      *(uint *)(piVar8[0xbf] + 0x3ec) = *(uint *)(piVar8[0xbf] + 0x3ec) | 0x10;
      EntityActivitySceneStateUpdate();
    }
    if (((piVar8[0xf9] & 0x10000U) != 0) &&
       (iVar10 = EntityProgressAndNegativeDeltaHandler(piVar8), iVar10 != 0)) {
      piVar8[0xf9] = piVar8[0xf9] & 0xfff7ffff;
    }
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      if (((piVar8[0xf9] & 0x20000U) == 0) &&
         (iVar10 = EntityProgressAndNegativeDeltaHandler(piVar8), iVar10 != 0)) {
        piVar8[0xf9] = piVar8[0xf9] & 0xfff7ffff;
        EntityCommandUnlinkByKind(piVar8,10,0,1);
      }
      if ((piVar8[0xf9] & 0x80000U) == 0) {
        piVar8[0xf8] = piVar8[0xf8] & 0xfeffffff;
        if (piVar8[0xbf] != 0) {
          puVar1 = (uint *)(piVar8[0xbf] + 0x3ec);
          *puVar1 = *puVar1 & 0xffffffef;
          EntityActivitySceneStateUpdate();
          piVar8[0xbf] = 0;
        }
        EntityCommandUnlinkByKind(piVar8,10,0,1);
      }
      else {
        uVar13 = EntityTypeComponentGet(piVar8);
        DDiSceneFrameUpdateAndCallbacks(uVar13,piVar8[0xb5]);
      }
    }
    goto LAB_0043ed80;
  }
  if ((uVar12 & 0x40000000) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      iVar10 = EntityProgressAdvanceBy25(piVar8);
      if (iVar10 == 0) {
        uVar13 = EntityTypeComponentGet(piVar8);
        DDiSceneFrameUpdateAndCallbacks(uVar13,piVar8[0xb5]);
      }
      else {
        piVar8[0xf8] = piVar8[0xf8] & 0xbfffffff;
        EntityCommandUnlinkByKind(piVar8,0x10,0,1);
        EntityCommandUnlinkByKind(piVar8,0x11,0,1);
      }
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x40) != 0) {
    piVar8[0xf9] = uVar6 & 0xffffffbf;
    if (piVar8[0xbc] != 0) {
      EntityActivityListAppend(piVar8,*(undefined4 *)(piVar8[0xbc] + 0x38));
      EntityCommandUnlinkByKind(piVar8,0x13,0,1);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x400) != 0) {
    if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
      piVar8[0xf9] = uVar6 & 0xfffffbff;
      EntityCommandUnlinkByKind(piVar8,0x17,0,1);
    }
    goto LAB_0043ed80;
  }
  if ((uVar6 & 0x8000) == 0) {
    if ((uVar12 & 0x80000) != 0) {
      uVar13 = EntityTypeComponentGet(piVar8);
      FUN_00475300(uVar13);
      if ((((piVar8[0xf9] & 0x10000U) != 0) && ((piVar8[0xf8] & 0x400U) != 0)) &&
         (piVar8[0xc2] != 0)) {
        uVar13 = EntityTypeComponentGet(piVar8[0xc2],0,&fStack_c);
        DDiSceneMoveOrPose(uVar13);
        if (*(int *)piVar8[0xc2] == 7) {
          uVar13 = 0x24;
        }
        else if (*(int *)piVar8[0xc2] == 6) {
          uVar13 = 0x25;
        }
        else {
          uVar13 = 0x23;
        }
        SoundSubmissionRouteWrapper(0,uVar13,0,0,&fStack_c);
      }
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = piVar8[0xf8] & 0xfff7ffff;
        FUN_00439e70(piVar8,1);
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x1000) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = uVar12 & 0xffffefff;
      }
      goto LAB_0043ed80;
    }
    if (piVar8[0xc1] != 0) goto LAB_0043ed80;
    if (((uVar12 & 1) != 0) && (iVar10 = FUN_0043c3e0(piVar8), iVar10 != 0)) {
      FUN_00441940(piVar8,1);
      EntityMarkAndFinishState1A(piVar8);
      goto LAB_0043ed80;
    }
    uVar12 = piVar8[0xf8];
    if ((uVar12 & 1) != 0) {
      FUN_004438b0(piVar8,fVar2);
      FUN_004440f0(piVar8,fVar2);
      FUN_0043aab0(piVar8);
      uVar12 = piVar8[0xf9];
      if ((uVar12 & 0x4000000) != 0) {
        iVar10 = FUN_00472080(piVar8,*(undefined4 *)(piVar8[0xbc] + 0x10));
        if ((iVar10 == 0) ||
           (iVar10 = FUN_004718d0(piVar8,*(undefined4 *)(piVar8[0xbc] + 0x10)), iVar10 != 0)) {
          piVar8[0xf9] = piVar8[0xf9] & 0xfbffffff;
        }
        else {
          EntityFinishState1A(piVar8);
        }
        goto LAB_0043ed80;
      }
      uVar16 = piVar8[0xf8];
      if ((uVar16 & 8) != 0) {
        fStack_c = (float)__ftol();
        fStack_8 = (float)__ftol();
        EntityActivityRequestSet(piVar8,4,0);
        TerrainCellPredugEdgeWallFlagSet(&fStack_c,1);
        goto LAB_0043ed80;
      }
      if ((uVar16 & 2) != 0) {
        EntityActivityRequestSet(piVar8,8,0);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 0x400) != 0) {
        EntityActivityRequestSet(piVar8,0x32,0);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 0x10000000) != 0) {
        EntityActivityRequestSet(piVar8,0x30,0);
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x40000) != 0) {
        EntityActivityRequestSet(piVar8,0x1e,(*piVar8 != 2) - 1U & 2);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 0x80000000) != 0) {
        EntityActivityRequestSet(piVar8,0x33,0);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 4) != 0) {
        if (piVar8[0xbf] != 0) {
          LinkedEntityRouteFlagClear(piVar8,piVar8[0xbf]);
        }
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x40) != 0) {
        fStack_c = (float)__ftol();
        fStack_8 = (float)__ftol();
        TerrainCellPredugEdgeWallFlagSet(&fStack_c,1);
        EntityActivityRequestSet(piVar8,7,DAT_00556f08);
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x80000) != 0) {
        EntityActivityRequestSet(piVar8,0x1f,0);
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x20000) != 0) {
        fStack_20 = (float)__ftol();
        iStack_1c = __ftol();
        uVar12 = EntityStateFlagsGet(piVar8);
        if (((uVar12 & 0x100000) == 0) &&
           (iVar10 = TerrainCellWalkableFlagCheck(fStack_20,iStack_1c), iVar10 == 0)) {
          piVar8[0xf8] = piVar8[0xf8] & 0xfffdffff;
          piVar8[0xf9] = piVar8[0xf9] | 0x20;
        }
        else {
          iVar10 = FUN_00432d90(fStack_20,iStack_1c);
          if (iVar10 == 0) {
            EntityActivityRequestSet(piVar8,0xd,0);
          }
          else {
            EntityActivityRequestSet(piVar8,0xe,0);
            iVar10 = SceneActivityUpdateDispatch(piVar8);
            if (iVar10 == 0) {
              EntityActivityRequestSet(piVar8,0xd,0);
              SceneActivityUpdateDispatch(piVar8);
            }
            uVar12 = EntityStateFlagsGet(piVar8);
            if ((uVar12 & 0x10000000) != 0) {
              TerrainCellFlag20ClearAndRefresh(&fStack_20);
            }
          }
        }
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x1000000) != 0) {
        EntityActivityRequestSet(piVar8,0x20,0);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 0x200000) != 0) {
        EntityActivityRequestSet(piVar8,0x24,0);
        goto LAB_0043ed80;
      }
      if ((uVar12 & 0x8000) != 0) {
        if (*piVar8 == 2) {
          EntityActivityRequestSet(piVar8,0x32,100);
        }
        else {
          EntityActivityRequestSet(piVar8,0x29,0);
          EntityActivityRequestSet(piVar8[0xbf],0x29,0);
          SceneActivityUpdateDispatch(piVar8[0xbf]);
          *(uint *)(piVar8[0xbf] + 0x3e4) = *(uint *)(piVar8[0xbf] + 0x3e4) | 0x8000;
        }
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x40000000) != 0) {
        EntityActivityRequestSet(piVar8,0x23,0);
        goto LAB_0043ed80;
      }
      if ((uVar16 & 0x200) == 0) {
        if ((uVar16 & 0x2000) != 0) {
          EntityActivityRequestSet(piVar8,0x1d,0);
          iVar10 = piVar8[0xbf];
          if ((iVar10 != 0) &&
             ((uVar12 = EntityStatsWordGet(iVar10), (uVar12 & 4) != 0 ||
              (uVar12 = EntityStatsWordGet(iVar10), (uVar12 & 2) != 0)))) {
            iVar10 = piVar8[0xbf];
            if (((*(uint *)(iVar10 + 0x3e0) & 0x2000) == 0) &&
               (((*(uint *)(iVar10 + 1000) & 0x1000000) == 0 &&
                (*(int *)(iVar10 + 0x324) != *(int *)(iVar10 + 0x328))))) {
              *(uint *)(iVar10 + 1000) = *(uint *)(iVar10 + 1000) | 0x1000000;
            }
            else {
              piVar8[0xf8] = piVar8[0xf8] & 0xffffdfffU | 0x4000;
            }
          }
          goto LAB_0043ed80;
        }
        if ((uVar16 & 0x800) != 0) {
          EntityActivityRequestSet(piVar8,0x13,0);
          goto LAB_0043ed80;
        }
        if (((uVar16 & 4) != 0) || ((uVar16 & 0x20000000) != 0)) goto LAB_0043ed80;
        if ((uVar16 & 0x400) != 0) {
          EntityPositionToTerrainCell(piVar8,&fStack_20,&iStack_1c);
          uVar12 = FUN_00432c50(&fStack_20);
          if (((uVar12 < 3) &&
              (*(char *)(*(int *)(DAT_005560d4 + 0xb8) + 2 +
                        (*(int *)(DAT_005560d4 + 0x14) * iStack_1c + (int)fStack_20) * 0x48) != '\t'
              )) || ((piVar8[0xfa] & 0x10000000U) == 0)) {
            EntityActivityRequestSet(piVar8,0x11,0);
          }
          else {
            EntityActivityRequestSet(piVar8,0x12,0);
          }
          goto LAB_0043ed80;
        }
        if ((uVar16 & 0x80000000) != 0) {
          FUN_00441940(piVar8,0);
          EntityActivityRequestSet(piVar8,0xb,1);
          goto LAB_0043ed80;
        }
        if ((uVar16 & 1) == 0) goto LAB_0043ed80;
        EntityPositionToTerrainCell(piVar8,&fStack_20,&iStack_1c);
        if (((piVar8[0xf8] & 0x800000U) != 0) && (*piVar8 == 2)) {
          EntityActivityRequestSet(piVar8,3,0);
          goto LAB_0043ed80;
        }
        uVar12 = FUN_00432c50(&fStack_20);
        if (((2 < uVar12) ||
            (*(char *)(*(int *)(DAT_005560d4 + 0xb8) + 2 +
                      (*(int *)(DAT_005560d4 + 0x14) * iStack_1c + (int)fStack_20) * 0x48) == '\t'))
           && ((piVar8[0xfa] & 0x10000000U) != 0)) {
          EntityActivityRequestSet(piVar8,2,0);
          goto LAB_0043ed80;
        }
        goto LAB_0043e472;
      }
      if ((piVar8[0xbf] == 0) || (*(int *)(piVar8[0xbf] + 0x304) != 0)) {
        piVar8[0xf8] = uVar16 & 0xfffffdff;
        goto LAB_0043ed80;
      }
      EntityActivityRequestSet(piVar8,0xf,0);
      if (*piVar8 == 3) {
        if (*(int *)piVar8[0xbf] == 5) {
          uVar13 = 0x10;
        }
        else {
          if (*(int *)piVar8[0xbf] != 6) goto LAB_0043e1f7;
          uVar13 = 0x23;
        }
        EntityActivityRequestSet(piVar8,uVar13,0);
      }
LAB_0043e1f7:
      piVar8[piVar8[0xc9] + 0xc2] = piVar8[0xbf];
      piVar8[0xbf] = 0;
      *(int **)(piVar8[piVar8[0xc9] + 0xc2] + 0x304) = piVar8;
      *(int *)(piVar8[piVar8[0xc9] + 0xc2] + 800) = piVar8[0xc9];
      piVar8[0xf8] = piVar8[0xf8] | 0x400;
      EntityCommandUnlinkByKind(piVar8,3,piVar8[piVar8[0xc9] + 0xc2],1);
      EntityActivityListRegister(piVar8);
      piVar8[0xc9] = piVar8[0xc9] + 1;
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x200000) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = uVar12 & 0xffdfffff;
        if (*piVar8 == 1) {
          EntityActivityRequestSet(piVar8,1,0);
          SceneActivityUpdateDispatch(piVar8);
        }
        EntityActivityRequestSet(piVar8,0,0);
        FUN_00438460(piVar8);
      }
      FUN_00442f80(piVar8,fVar2);
      iVar10 = EntityTypeComponentGet(piVar8);
      if (iVar10 != 0) {
        FUN_00475500(iVar10,0,&fStack_c,0);
        FUN_00475470(iVar10,0,fStack_c,fStack_8,fStack_4,0,0,0xbf800000);
      }
      goto LAB_0043ed80;
    }
    iVar10 = FUN_00449980(piVar8,fVar2,&iStack_14);
    if (iVar10 != 0) {
      if (iStack_14 != 0) {
        EntityCommandUnlinkByKind(piVar8,0xb,0,1);
      }
      goto LAB_0043ed80;
    }
    uVar12 = piVar8[0xf8];
    if ((uVar12 & 0x100) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = uVar12 & 0xfffffeff;
        iVar10 = EntityType3Flag04000000Test(piVar8);
        if (iVar10 != 0) {
          fStack_c = (float)__ftol();
          fStack_8 = (float)__ftol();
          KeyStateGatedCommandEnqueue(0x20,piVar8,0,0);
          TerrainCellPredugEdgeWallFlagSet(&fStack_c,0);
        }
      }
      goto LAB_0043ed80;
    }
    uVar16 = piVar8[0xf9];
    if ((uVar16 & 0x100000) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf9] = uVar16 & 0xffefffff;
        piVar8[0xfa] = piVar8[0xfa] | 0x800000;
      }
      goto LAB_0043ed80;
    }
    if ((uVar16 & 4) != 0) {
      iVar10 = piVar8[0xbf];
      piVar8[0xf9] = uVar16 & 0xfffffffb;
      EntityCommandUnlinkByKind(piVar8,0x12,0,1);
      piVar8[0xf9] = piVar8[0xf9] | 8;
      *(uint *)(iVar10 + 0x3ec) = *(uint *)(iVar10 + 0x3ec) & 0xffffff7f;
      if (*(int *)(iVar10 + 0x2fc) != 0) {
        puVar1 = (uint *)(*(int *)(iVar10 + 0x2fc) + 0x3ec);
        *puVar1 = *puVar1 & 0xffffff7f;
      }
      *(undefined4 *)(iVar10 + 0x2fc) = 0;
      *(uint *)(iVar10 + 0x3e4) = *(uint *)(iVar10 + 0x3e4) & 0xffffffef;
      piVar8[0xdb] = iVar10;
      *(int **)(iVar10 + 0x36c) = piVar8;
      piVar8[0xbf] = 0;
      SelectionObjectCommandUpdate(piVar8[0xdb]);
      FUN_00442f80(piVar8,fVar2);
      *(undefined4 *)(iVar10 + 0x2ec) = 0;
      SceneActivityUpdateDispatch(iVar10);
      goto LAB_0043ed80;
    }
    if ((uVar16 & 8) != 0) goto LAB_0043ed80;
    if ((uVar12 & 0x20000) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = uVar12 & 0xfffdffff;
        piVar8[0xf9] = uVar16 & 0xffffffdf;
        EntityDetachCarriedObjectAndResetState(piVar8);
      }
      goto LAB_0043ed80;
    }
    if ((uVar16 & 0x200) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        EntityPositionToTerrainCell(piVar8,&fStack_20,&iStack_1c);
        iVar10 = TerrainDescriptorFindExact(&fStack_20,0);
        if ((iVar10 != 0) && ((*(byte *)(iVar10 + 0xd0) & 2) != 0)) {
          TerrainCellVariantFlagPropagate(&fStack_20);
          ReleaseSelectionGroupResources(&fStack_20);
          MessageDispatch(0x20,0,0,&fStack_20);
          TaskPairRecordEnqueue(&fStack_20);
        }
        piVar8[0xf9] = piVar8[0xf9] & 0xfffffdff;
        EntityCommandUnlinkByKind(piVar8,0x16,0,1);
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x40000) != 0) {
      if (*piVar8 == 1) {
        FUN_00442f80(piVar8,fVar2);
      }
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = piVar8[0xf8] & 0xfffbffff;
        EntityCommandUnlinkByKind(piVar8,0xc,0,1);
        if (*piVar8 == 1) {
          EntityActivityRequestSet(piVar8,0,0);
        }
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x80000000) != 0) goto LAB_0043e851;
    if ((uVar16 & 0x100) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf9] = uVar16 & 0xfffffeff;
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x10000) != 0) {
      sVar9 = RandomNext();
      if ((int)sVar9 % 10 != 0) {
        FUN_00444260(piVar8);
      }
      piVar8[0xf8] = piVar8[0xf8] & 0xfffeffff;
      goto LAB_0043ed80;
    }
    if ((uVar16 & 2) != 0) {
      if (((*(byte *)(piVar8[0xf0] + 0x3e4) & 1) == 0) || (_DAT_0049e3a4 < (float)piVar8[0xb5])) {
        piVar8[0xf9] = uVar16 & 0xfffffffd;
        EntityActivityRequestSet(piVar8,0x27,0);
        SceneActivityUpdateDispatch(piVar8);
        piVar8[0xf9] = piVar8[0xf9] | 0x100;
      }
      goto LAB_0043ed80;
    }
    if ((uVar16 & 1) != 0) {
      if ((uVar16 & 0x10000) != 0) {
        FUN_00442260(piVar8,&fStack_c,&fStack_8);
        fVar17 = (float10)VectorDirectionAngleRadiansGet(piVar8,1);
        EntityWorldPositionSet(piVar8[0xf0],fStack_c,fStack_8,(float)fVar17);
      }
      if ((float)piVar8[0xb5] <= _DAT_0049e3a4) goto LAB_0043ed80;
      uVar13 = EntityVisualResourceGetOrCreate(piVar8);
      DDiSceneMoveOrPose(uVar13,0,&fStack_c);
      EntityWorldPositionSet(piVar8,fStack_c,fStack_8,0,0);
      piVar8[0xf9] = piVar8[0xf9] & 0xfffffffe;
      EntityDamageApply(piVar8[0xf0],0x41200000,1,fVar2);
      goto LAB_0043e9ee;
    }
    iVar10 = FUN_00433c10(piVar8,fVar2,0);
    if (iVar10 != 0) goto LAB_0043ed80;
    iVar10 = FUN_00442b10(piVar8,fVar2,&fStack_10);
    if (iVar10 != 0) {
      if (iVar10 == 1) {
        if ((piVar8[0xf8] & 0x400U) == 0) {
          EntityActivityRequestSet(piVar8,1,0);
        }
        else {
          EntityActivityRequestSet(piVar8,0x11,0);
        }
      }
      else if (iVar10 == 4) {
        EntityActivityRequestSet(piVar8,0,0);
      }
      else if (iVar10 == 2) {
        EntityActivityRequestSet(piVar8,6,0);
      }
      else if ((iVar10 == 3) &&
              (iVar10 = EntityWorkObjectPositionGet(piVar8,&fStack_20,&param_1), iVar10 != 0)) {
        uVar13 = FUN_00431570(fStack_20,param_1,&fStack_30,&fStack_2c);
        iVar10 = TerrainGridPick(uVar13);
        if ((iVar10 != 0) &&
           (iVar10 = TerrainCellWalkableFlagCheck(fStack_30,fStack_2c), iVar10 != 0)) {
          uVar13 = TerrainCellMaterialGet(fStack_30,fStack_2c);
          fVar17 = (float10)TerrainTypeMovementSpeedGet(piVar8,uVar13);
          fStack_24 = (float)fVar17;
          if ((piVar8[0xfb] & 0x20000U) == 0) {
            EntityTypeComponentGet(piVar8);
            uVar13 = NestedWordAtOffsetC0OrC4Get(piVar8,0,1);
            iVar10 = SoundAliasPlay(uVar13);
            piVar8[0xd4] = iVar10;
            piVar8[0xfb] = piVar8[0xfb] | 0x120000;
          }
          bVar7 = false;
          EntityActivityRequestSet(piVar8,4,0);
          iVar10 = TerrainCellProgressUpdate(fStack_30,fStack_2c,fStack_24,fVar2);
          if ((iVar10 != 0) &&
             (iVar10 = TerrainCellWalkabilityQuery(piVar8,fStack_30,fStack_2c,0,1), iVar10 != 0)) {
            uVar13 = CurrentLevelGet(fStack_30,fStack_2c,0);
            TerrainCellMutation(uVar13);
          }
        }
      }
      if (*piVar8 == 2) {
        if ((DAT_004eadcc & 3) == 0) {
          SoundAliasPlay(8,0);
        }
        DAT_004eadcc = DAT_004eadcc + 1;
      }
      goto LAB_0043ed80;
    }
    uVar12 = piVar8[0xf8];
    if ((uVar12 & 0x200) != 0) {
      if ((_DAT_0049e3a4 < (float)piVar8[0xb5]) &&
         (piVar8[0xf8] = uVar12 & 0xfffffdff, (uVar12 & 0x400) != 0)) {
        if (*piVar8 == 3) {
          piVar3 = (int *)piVar8[0xc2];
          if (*piVar3 == 5) {
            uVar13 = FUN_00438b80(piVar8,0x400);
            KeyStateGatedCommandEnqueue(0x22,piVar8,uVar13,0);
          }
          else {
            piVar8[0xc2] = 0;
            piVar8[0xc9] = 0;
            piVar3[0xc1] = 0;
            piVar3[0xc0] = 0;
            EntityCommandUnlinkByKind(piVar8,3,piVar3,1);
            EntityActionRecordConsumeAndCleanup(piVar8,piVar3);
            piVar8[0xf8] = piVar8[0xf8] & 0xfffffbff;
          }
        }
        else if (*(int *)piVar8[piVar8[0xc9] + 0xc1] == 6) {
          GlobalObjectField94Increment();
        }
        else if (*(int *)piVar8[piVar8[0xc9] + 0xc1] == 7) {
          GlobalObjectFieldA8Increment();
          MessageDispatch(0x1a,0,piVar8,0);
        }
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 0x8000) != 0) {
      if (_DAT_0049e3a4 < (float)piVar8[0xb5]) {
        piVar8[0xf8] = uVar12 & 0xffff7fff;
      }
      goto LAB_0043ed80;
    }
    if ((uVar12 & 4) != 0) goto LAB_0043ed80;
    if ((uVar12 & 0x400) == 0) {
      iVar10 = *piVar8;
      if (iVar10 == 4) {
        if ((piVar8[0xfa] & 0x20000000U) != 0) {
          if ((piVar8[0xfa] & 0x80000000U) != 0) {
            uVar13 = 0x2b;
            goto LAB_0043ed69;
          }
          goto LAB_0043ed38;
        }
      }
      else {
LAB_0043ed38:
        if ((iVar10 != 3) || ((piVar8[0xfa] & 0x80000000U) == 0)) {
          if ((iVar10 == 1) && ((piVar8[0xfb] & 0x2000U) != 0)) {
            uVar13 = 0x37;
          }
          else {
            uVar13 = 0;
          }
          goto LAB_0043ed69;
        }
      }
      uVar13 = 0x2b;
    }
    else {
      uVar13 = 0x16;
    }
LAB_0043ed69:
    EntityActivityRequestSet(piVar8,uVar13,0);
    FUN_00442f80(piVar8,fVar2);
    goto LAB_0043ed80;
  }
  if (*piVar8 == 2) {
    if (piVar8[0xbf] == 0) {
      piVar8[0xf9] = uVar6 & 0xffff7fff;
    }
    else {
      fVar17 = (float10)AnimationObjectTimeGet(piVar8[4]);
      fStack_20 = (float)fVar17;
      fVar17 = (float10)Entity334FloatFieldF4Get(piVar8);
      param_1 = (int *)(float)fVar17;
      fVar17 = (float10)FUN_0046a0a0(piVar8[0xbf]);
      if (fVar17 * (float10)(float)param_1 <= (float10)fStack_20) {
        EntityCommandUnlinkByKind(piVar8,0x15,0,1);
        EntityActivityRequestSet(piVar8,0,0);
        SceneActivityUpdateDispatch(piVar8);
        iVar10 = piVar8[0xcc];
        uVar12 = BuildingUpgradeAdmissionCheck(*piVar8,piVar8[1]);
        if (iVar10 + 1U < uVar12) {
          EntityBuildingIndexSet(piVar8,iVar10 + 1U);
          BuildingUpgradeStateChangedEventDispatch(*piVar8,piVar8[1],piVar8[0xcc],0);
        }
        piVar8[0xbf] = 0;
        piVar8[0xf9] = piVar8[0xf9] & 0xffff7fff;
      }
    }
    goto LAB_0043ed80;
  }
  if ((float)piVar8[0xb5] <= _DAT_0049e3a4) goto LAB_0043ed80;
  if ((piVar8[0xbf] == 0) || (uVar12 = EntityStateFlagsGet(piVar8[0xbf]), (uVar12 & 2) == 0)) {
    piVar8[0xf9] = piVar8[0xf9] & 0xffff7fff;
    goto LAB_0043ed80;
  }
  if ((*(uint *)(piVar8[0xbf] + 0x3e4) & 0x8000) != 0) goto LAB_0043ed80;
  EntityCommandUnlinkByKind(piVar8,0x15,0,1);
  uVar12 = MappedIndexedRecordWordAtFCGet(*piVar8,piVar8[1],piVar8[0x101]);
  uVar16 = UpgradeResourceCheck(0);
  if (uVar16 < uVar12) {
    uVar12 = MappedIndexedRecordWordAt100Get(*piVar8,piVar8[1],piVar8[0x101]);
    uVar16 = UpgradeResourceCheck(1);
    if (uVar12 <= uVar16) {
      EntityIndexedFlagSetAndRefresh(piVar8);
      uVar13 = MappedIndexedRecordWordAt100Get(*piVar8,piVar8[1],piVar8[0x101]);
      uVar14 = 1;
      goto LAB_0043dcb3;
    }
  }
  else {
    EntityIndexedFlagSetAndRefresh(piVar8);
    uVar13 = MappedIndexedRecordWordAtFCGet(*piVar8,piVar8[1],piVar8[0x101]);
    uVar14 = 0;
LAB_0043dcb3:
    UpgradeResourceDebit(uVar14,uVar13);
  }
  *(undefined4 *)(piVar8[0xbf] + 0x300) = 0;
  piVar8[0xbf] = 0;
  piVar8[0xf9] = piVar8[0xf9] & 0xffff7fff;
LAB_0043ed80:
  if ((bVar7) && ((piVar8[0xfb] & 0x20000U) != 0)) {
    EntityTypeComponentGet(piVar8);
    SoundHandleStopForward(piVar8[0xd4]);
    piVar8[0xfb] = piVar8[0xfb] & 0xffefffff;
    uVar13 = NestedWordAtOffsetC0OrC4Get(piVar8,1,0);
    SoundAliasPlay(uVar13);
    piVar8[0xfb] = piVar8[0xfb] & 0xfffdffff;
  }
  SceneActivityUpdateDispatch(piVar8);
  if ((piVar8[0xdb] != 0) && (*piVar8 != 2)) {
    FUN_00443510(piVar8);
  }
  if ((piVar8[0xf8] & 0x400U) != 0) {
    AttachedEntityDirectionVectorUpdate(piVar8);
  }
  iVar10 = EntityFlag100AndSharedEligibilityCheck(piVar8);
  if (iVar10 != 0) {
    EntityVelocitySlotDispatch(piVar8,fVar2);
  }
  FUN_004456e0(piVar8,fVar2);
  FUN_0044bc30(piVar8,*param_2);
  EntityEnergyLowTargetRouteCommandTick(piVar8,fVar2);
  if ((piVar8[0xf9] & 0x10000U) != 0) {
    piVar8[0xf9] = piVar8[0xf9] & 0xfffeffffU | 0x20000;
  }
  piVar8[0xfb] = piVar8[0xfb] & 0xfffffffd;
  ObjectiveSidecarAppendGate(piVar8);
  return 0;
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
          GlobalRenderRecordSetFromInput(DAT_004fef2c);
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
          GlobalRenderRecordSetFromInput(DAT_004fef2c);
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
          GlobalRenderRecordSetFromInput(DAT_004fef2c);
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
// ==== SetTutorialBlockIsPath @ 00457420 ====

undefined4 SetTutorialBlockIsPath(undefined4 *param_1)

{
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;

  uStack_2c = 4;
  uStack_28 = *param_1;
  uStack_24 = 0;
  CoordinateListCallbackVisit(uStack_28,uRam004a676c,&uStack_2c);
  return uStack_24;
}
// ==== ObjectiveEntityPairedFieldPredicate @ 00459100 ====

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 ObjectiveEntityPairedFieldPredicate(int *param_1,int param_2)

{
  int iStack_8;
  int iStack_4;

  if (((_DAT_004ffbc0 & 0x400) != 0) && ((*param_1 == 2 || (*param_1 == 1)))) {
    EntityPositionToTerrainCell(param_1,&iStack_8,&iStack_4);
    if ((iStack_8 == *(int *)(param_2 + 0x40)) && (iStack_4 == *(int *)(param_2 + 0x44))) {
      return 1;
    }
  }
  if ((((_DAT_004ffbc0 & 0x1000) != 0) && (*(int *)(param_2 + 0x4c) == *param_1)) &&
     (*(int *)(param_2 + 0x50) == param_1[1])) {
    return 1;
  }
  return 0;
}
