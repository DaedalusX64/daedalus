//#define DISABLE_IOFUNCS
//#define DISABLE_CPU_FUNCTIONS
//#define DISABLE_GUFUNCS
//#define DISABLE_THREAD_FUNCS
//#define DISABLE_LLFUNCS
//#define DISABLE_CSTDFUNCS
//#define DISABLE_SP_FUNCTIONS
//#define DISABLE_MSG_FUNCTIONS
//#define DISABLE_TIMER_FUNCTIONS 

////////////////////////////////////////////////////////////
//                 osStartThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osStartThread)
  PATCH_XREF_FUNCTION(4, __osDisableInt)
  PATCH_XREF_VAR_HI(16, osThreadQueue)
  PATCH_XREF_FUNCTION(19, __osEnqueueThread)
  PATCH_XREF_VAR_LO(20, osThreadQueue)
  PATCH_XREF_VAR_HI(27, osThreadQueue)
  PATCH_XREF_VAR_LO(28, osThreadQueue)
  PATCH_XREF_VAR_HI(33, osThreadQueue)
  PATCH_XREF_FUNCTION(36, __osEnqueueThread)
  PATCH_XREF_VAR_LO(37, osThreadQueue)
  PATCH_XREF_FUNCTION(45, __osEnqueueThread)
  PATCH_XREF_FUNCTION(48, __osPopThread)
  PATCH_XREF_VAR_HI(51, osThreadQueue)
  PATCH_XREF_VAR_LO(52, osThreadQueue)
  PATCH_XREF_FUNCTION(53, __osEnqueueThread)
  PATCH_XREF_VAR_HI(55, osActiveThread)
  PATCH_XREF_VAR_LO(56, osActiveThread)
  PATCH_XREF_FUNCTION(59, __osDispatchThread)
  PATCH_XREF_VAR_HI(63, osActiveThread)
  PATCH_XREF_VAR_HI(64, osThreadQueue)
  PATCH_XREF_VAR_LO(65, osThreadQueue)
  PATCH_XREF_VAR_LO(66, osActiveThread)
  PATCH_XREF_VAR_HI(73, osThreadQueue)
  PATCH_XREF_FUNCTION(75, __osEnqueueAndYield)
  PATCH_XREF_VAR_LO(76, osThreadQueue)
  PATCH_XREF_FUNCTION(77, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osStartThread)
   PATCH_SIGNATURE_LIST_ENTRY(osStartThread, 84, 9, 0x7f8ccff8, 0x67467a00)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osStartThread)


////////////////////////////////////////////////////////////
//               osDestroyThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osDestroyThread_Mario)
  PATCH_XREF_FUNCTION(5, __osDisableInt)
  PATCH_XREF_VAR_HI(11, osActiveThread)
  PATCH_XREF_VAR_LO(12, osActiveThread)
  PATCH_XREF_FUNCTION(21, __osDequeueThread)
  PATCH_XREF_VAR_HI(23, osGlobalThreadList)
  PATCH_XREF_VAR_LO(24, osGlobalThreadList)
  PATCH_XREF_VAR_HI(29, osGlobalThreadList)
  PATCH_XREF_VAR_LO(31, osGlobalThreadList)
  PATCH_XREF_VAR_HI(32, osGlobalThreadList)
  PATCH_XREF_VAR_LO(33, osGlobalThreadList)
  PATCH_XREF_VAR_HI(47, osActiveThread)
  PATCH_XREF_VAR_LO(48, osActiveThread)
  PATCH_XREF_FUNCTION(52, __osDispatchThread)
  PATCH_XREF_FUNCTION(54, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osDestroyThread_Zelda)
  PATCH_XREF_FUNCTION(5, __osDisableInt)
  PATCH_XREF_VAR_HI(11, osActiveThread)
  PATCH_XREF_VAR_LO(12, osActiveThread)
  PATCH_XREF_FUNCTION(21, __osDequeueThread)
  PATCH_XREF_VAR_HI(23, osGlobalThreadList)
  PATCH_XREF_VAR_LO(24, osGlobalThreadList)
  PATCH_XREF_VAR_HI(29, osGlobalThreadList)
  PATCH_XREF_VAR_LO(31, osGlobalThreadList)
  PATCH_XREF_VAR_HI(32, osGlobalThreadList)
  PATCH_XREF_VAR_LO(33, osGlobalThreadList)
  PATCH_XREF_VAR_HI(50, osActiveThread)
  PATCH_XREF_VAR_LO(51, osActiveThread)
  PATCH_XREF_FUNCTION(55, __osDispatchThread)
  PATCH_XREF_FUNCTION(57, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osDestroyThread)
   PATCH_SIGNATURE_LIST_ENTRY(osDestroyThread_Mario, 62, 9, 0xe987033a, 0x75132538)
   PATCH_SIGNATURE_LIST_ENTRY(osDestroyThread_Zelda, 65, 9, 0xe987033a, 0x45c97cae)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osDestroyThread)


////////////////////////////////////////////////////////////
//                    osContInit
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osContInit)
  PATCH_XREF_VAR_HI(1, osContInitialised)
  PATCH_XREF_VAR_LO(2, osContInitialised)
  PATCH_XREF_VAR_HI(12, osContInitialised)
  PATCH_XREF_FUNCTION(13, osGetTime)
  PATCH_XREF_VAR_LO(14, osContInitialised)
  PATCH_XREF_VAR_HI(16, osClockRateHi)
  PATCH_XREF_VAR_HI(17, osClockRateLo)
  PATCH_XREF_VAR_LO(20, osClockRateLo)
  PATCH_XREF_VAR_LO(21, osClockRateHi)
  PATCH_XREF_FUNCTION(23, __ull_mul)
  PATCH_XREF_FUNCTION(31, __ull_div)
  PATCH_XREF_VAR_HI(47, osClockRateHi)
  PATCH_XREF_VAR_HI(48, osClockRateLo)
  PATCH_XREF_VAR_LO(49, osClockRateLo)
  PATCH_XREF_VAR_LO(50, osClockRateHi)
  PATCH_XREF_FUNCTION(52, __ull_mul)
  PATCH_XREF_FUNCTION(60, __ull_div)
  PATCH_XREF_FUNCTION(80, osSetTimer)
  PATCH_XREF_FUNCTION(84, osRecvMesg)
  PATCH_XREF_VAR_HI(87, osNumControllers)
  PATCH_XREF_VAR_LO(88, osNumControllers)
  PATCH_XREF_FUNCTION(89, __osPackRequestData)
  PATCH_XREF_VAR_HI(91, osPackRequestDataBuffer)
  PATCH_XREF_VAR_LO(92, osPackRequestDataBuffer)
  PATCH_XREF_FUNCTION(93, __osSiRawStartDma)
  PATCH_XREF_FUNCTION(98, osRecvMesg)
  PATCH_XREF_VAR_HI(100, osPackRequestDataBuffer)
  PATCH_XREF_VAR_LO(101, osPackRequestDataBuffer)
  PATCH_XREF_FUNCTION(102, __osSiRawStartDma)
  PATCH_XREF_FUNCTION(107, osRecvMesg)
  PATCH_XREF_FUNCTION(110, __osContGetInitData)
  PATCH_XREF_VAR_HI(113, osContInitUnk)
  PATCH_XREF_FUNCTION(114, __osSiCreateAccessQueue)
  PATCH_XREF_VAR_LO(115, osContInitUnk)
  PATCH_XREF_VAR_HI(116, osEepromMQ)
  PATCH_XREF_VAR_HI(117, osEepromMsgBuffer)
  PATCH_XREF_VAR_LO(118, osEepromMsgBuffer)
  PATCH_XREF_VAR_LO(119, osEepromMQ)
  PATCH_XREF_FUNCTION(120, osCreateMesgQueue)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osContInit)
   PATCH_SIGNATURE_LIST_ENTRY(osContInit, 127, 9, 0x437be57e, 0xa565710e)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osContInit)


////////////////////////////////////////////////////////////
//                  osSetIntMask
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSetIntMask)
  PATCH_XREF_VAR_HI(2, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(3, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(23, osDispatchThreadRCPThingamy)
  PATCH_XREF_VAR_LO(25, osDispatchThreadRCPThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSetIntMask)
   PATCH_SIGNATURE_LIST_ENTRY(osSetIntMask, 40, 16, 0x688655fd, 0xac1edb28)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSetIntMask)


////////////////////////////////////////////////////////////
//                    osSetTimer
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSetTimer)
  PATCH_XREF_FUNCTION(35, __osInsertTimer)
  PATCH_XREF_VAR_HI(37, osTopTimer)
  PATCH_XREF_VAR_LO(38, osTopTimer)
  PATCH_XREF_FUNCTION(46, __osSetTimerIntr)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSetTimer)
   PATCH_SIGNATURE_LIST_ENTRY(osSetTimer, 53, 9, 0x5400912f, 0xa4f22d03)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSetTimer)


////////////////////////////////////////////////////////////
//               __osInsertTimer
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osInsertTimer)
  PATCH_XREF_FUNCTION(2, __osDisableInt)
  PATCH_XREF_VAR_HI(4, osTopTimer)
  PATCH_XREF_VAR_LO(5, osTopTimer)
  PATCH_XREF_VAR_HI(31, osTopTimer)
  PATCH_XREF_VAR_LO(39, osTopTimer)
  PATCH_XREF_VAR_HI(58, osTopTimer)
  PATCH_XREF_VAR_LO(61, osTopTimer)
  PATCH_XREF_FUNCTION(91, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osInsertTimer)
   PATCH_SIGNATURE_LIST_ENTRY(__osInsertTimer, 98, 9, 0x97b901ef, 0xd84ec875)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osInsertTimer)


////////////////////////////////////////////////////////////
//               osSpTaskStartGo
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSpTaskStartGo)
  PATCH_XREF_FUNCTION(2, __osSpDeviceBusy)
  PATCH_XREF_FUNCTION(6, __osSpDeviceBusy)
  PATCH_XREF_FUNCTION(10, __osSpSetStatus)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSpTaskStartGo)
   PATCH_SIGNATURE_LIST_ENTRY(osSpTaskStartGo, 16, 9, 0x20b4d5ac, 0xba6fc3e9)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSpTaskStartGo)


////////////////////////////////////////////////////////////
//                 osSpTaskYield
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSpTaskYield_Mario)
  PATCH_XREF_FUNCTION(2, __osSpSetStatus)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osSpTaskYield_Rugrats)
  PATCH_XREF_FUNCTION(2, __osSpSetStatus)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSpTaskYield)
   PATCH_SIGNATURE_LIST_ENTRY(osSpTaskYield_Mario, 8, 9, 0x667de441, 0xc75ba905)
   PATCH_SIGNATURE_LIST_ENTRY(osSpTaskYield_Rugrats, 7, 9, 0x657bdf3e, 0x2bf31753)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSpTaskYield)


////////////////////////////////////////////////////////////
//               osSpTaskYielded
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSpTaskYielded)
  PATCH_XREF_FUNCTION(2, __osSpGetStatus)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSpTaskYielded)
   PATCH_SIGNATURE_LIST_ENTRY(osSpTaskYielded, 32, 9, 0x66389aad, 0x9d6e4db8)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSpTaskYielded)


////////////////////////////////////////////////////////////
//                  osSpTaskLoad
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSpTaskLoad)
  PATCH_XREF_FUNCTION(3, __osSpTaskLoadInitTask)
  PATCH_XREF_FUNCTION(22, osWritebackDCache)
  PATCH_XREF_FUNCTION(24, __osSpSetStatus)
  PATCH_XREF_FUNCTION(27, __osSpSetPc)
  PATCH_XREF_FUNCTION(33, __osSpSetPc)
  PATCH_XREF_FUNCTION(42, __osSpRawStartDma)
  PATCH_XREF_FUNCTION(51, __osSpRawStartDma)
  PATCH_XREF_FUNCTION(56, __osSpDeviceBusy)
  PATCH_XREF_FUNCTION(60, __osSpDeviceBusy)
  PATCH_XREF_FUNCTION(69, __osSpRawStartDma)
  PATCH_XREF_FUNCTION(79, __osSpRawStartDma)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSpTaskLoad)
   PATCH_SIGNATURE_LIST_ENTRY(osSpTaskLoad, 88, 9, 0x0fc26ac4, 0x98cea2fd)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSpTaskLoad)


////////////////////////////////////////////////////////////
//        __osSpTaskLoadInitTask
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpTaskLoadInitTask)
  PATCH_XREF_VAR_HI(1, osSpTaskLoadTempTask)
  PATCH_XREF_VAR_LO(4, osSpTaskLoadTempTask)
  PATCH_XREF_FUNCTION(8, bcopy)
  PATCH_XREF_FUNCTION(14, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(22, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(30, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(38, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(46, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(54, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(62, osVirtualToPhysical)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpTaskLoadInitTask)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpTaskLoadInitTask, 71, 9, 0xdfe0a9e5, 0x4c227799)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpTaskLoadInitTask)


////////////////////////////////////////////////////////////
//             __osSpRawStartDma
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpRawStartDma)
  PATCH_XREF_FUNCTION(5, __osSpDeviceBusy)
  PATCH_XREF_FUNCTION(14, osVirtualToPhysical)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpRawStartDma)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpRawStartDma, 35, 9, 0xac99ef9a, 0x73ad2920)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpRawStartDma)


////////////////////////////////////////////////////////////
//               __osSpSetStatus
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpSetStatus_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSpSetStatus_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpSetStatus)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpSetStatus_Mario, 4, 15, 0x54fadc24, 0x54fadc24)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpSetStatus_Rugrats, 4, 15, 0x24e78877, 0x24e78877)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpSetStatus)


////////////////////////////////////////////////////////////
//               __osSpGetStatus
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpGetStatus_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSpGetStatus_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpGetStatus)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpGetStatus_Mario, 4, 15, 0x4362103d, 0x4362103d)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpGetStatus_Rugrats, 4, 15, 0x49d30f39, 0x49d30f39)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpGetStatus)


////////////////////////////////////////////////////////////
//                   __osSpSetPc
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpSetPc)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpSetPc)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpSetPc, 13, 15, 0xfb8f235e, 0xdd02ba23)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpSetPc)


////////////////////////////////////////////////////////////
//              __osSpDeviceBusy
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSpDeviceBusy_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSpDeviceBusy_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSpDeviceBusy)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpDeviceBusy_Mario, 12, 15, 0x23bae380, 0x091f7a25)
   PATCH_SIGNATURE_LIST_ENTRY(__osSpDeviceBusy_Rugrats, 6, 15, 0x254c94aa, 0xefb18f4b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSpDeviceBusy)


////////////////////////////////////////////////////////////
//               __osSiRawReadIo
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiRawReadIo_Mario)
  PATCH_XREF_FUNCTION(3, __osSiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSiRawReadIo_Zelda)
  PATCH_XREF_FUNCTION(3, __osSiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiRawReadIo)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawReadIo_Mario, 20, 9, 0x2af9bd43, 0x2b3aaa41)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawReadIo_Zelda, 19, 9, 0x2af9bd43, 0x2e01f600)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiRawReadIo)


////////////////////////////////////////////////////////////
//              __osSiRawWriteIo
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiRawWriteIo_Mario)
  PATCH_XREF_FUNCTION(3, __osSiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSiRawWriteIo_Zelda)
  PATCH_XREF_FUNCTION(3, __osSiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiRawWriteIo)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawWriteIo_Mario, 19, 9, 0x2af9bd43, 0x68833de2)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawWriteIo_Zelda, 18, 9, 0x2af9bd43, 0x75bb0823)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiRawWriteIo)


////////////////////////////////////////////////////////////
//                  osEepromRead
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osEepromRead)
  PATCH_XREF_VAR_HI(3, osEepPifBuffer)
  PATCH_XREF_VAR_LO(4, osEepPifBuffer)
  PATCH_XREF_FUNCTION(15, __osSiGetAccess)
  PATCH_XREF_FUNCTION(18, __osEepStatus)
  PATCH_XREF_FUNCTION(35, __osEepStatus)
  PATCH_XREF_FUNCTION(41, __osEepromRead_Prepare)
  PATCH_XREF_VAR_HI(43, osEepPifBuffer)
  PATCH_XREF_VAR_LO(44, osEepPifBuffer)
  PATCH_XREF_FUNCTION(45, __osSiRawStartDma)
  PATCH_XREF_VAR_HI(55, osEepPifBuffer)
  PATCH_XREF_VAR_LO(59, osEepPifBuffer)
  PATCH_XREF_VAR_HI(64, osEepPifThingamy)
  PATCH_XREF_VAR_HI(65, osEepPifBuffer)
  PATCH_XREF_VAR_LO(66, osEepPifThingamy)
  PATCH_XREF_VAR_LO(67, osEepPifBuffer)
  PATCH_XREF_FUNCTION(68, __osSiRawStartDma)
  PATCH_XREF_VAR_HI(71, osEepPifThingamy2)
  PATCH_XREF_VAR_LO(73, osEepPifThingamy2)
  PATCH_XREF_FUNCTION(76, osRecvMesg)
  PATCH_XREF_FUNCTION(117, __osSiRelAccess)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osEepromRead)
   PATCH_SIGNATURE_LIST_ENTRY(osEepromRead, 124, 9, 0x5688a930, 0xd9bb15fd)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osEepromRead)


////////////////////////////////////////////////////////////
//                 osEepromWrite
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osEepromWrite)
  PATCH_XREF_VAR_HI(3, osEepPifBuffer)
  PATCH_XREF_VAR_LO(4, osEepPifBuffer)
  PATCH_XREF_FUNCTION(14, __osSiGetAccess)
  PATCH_XREF_FUNCTION(17, __osEepStatus)
  PATCH_XREF_FUNCTION(34, __osEepStatus)
  PATCH_XREF_FUNCTION(41, __osEepromWrite_Prepare)
  PATCH_XREF_VAR_HI(43, osEepPifBuffer)
  PATCH_XREF_VAR_LO(44, osEepPifBuffer)
  PATCH_XREF_FUNCTION(45, __osSiRawStartDma)
  PATCH_XREF_FUNCTION(50, osRecvMesg)
  PATCH_XREF_VAR_HI(55, osEepPifBuffer)
  PATCH_XREF_VAR_LO(59, osEepPifBuffer)
  PATCH_XREF_VAR_HI(64, osEepPifThingamy)
  PATCH_XREF_VAR_HI(65, osEepPifBuffer)
  PATCH_XREF_VAR_LO(66, osEepPifThingamy)
  PATCH_XREF_VAR_LO(67, osEepPifBuffer)
  PATCH_XREF_FUNCTION(68, __osSiRawStartDma)
  PATCH_XREF_VAR_HI(71, osEepPifThingamy2)
  PATCH_XREF_VAR_LO(73, osEepPifThingamy2)
  PATCH_XREF_FUNCTION(76, osRecvMesg)
  PATCH_XREF_FUNCTION(101, __osSiRelAccess)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osEepromWrite)
   PATCH_SIGNATURE_LIST_ENTRY(osEepromWrite, 108, 9, 0x5688a930, 0xb3b6fcd6)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osEepromWrite)


////////////////////////////////////////////////////////////
//                 osEepromProbe
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osEepromProbe)
  PATCH_XREF_FUNCTION(3, __osSiGetAccess)
  PATCH_XREF_FUNCTION(6, __osEepStatus)
  PATCH_XREF_FUNCTION(20, __osSiRelAccess)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osEepromProbe)
   PATCH_SIGNATURE_LIST_ENTRY(osEepromProbe, 27, 9, 0x0fc26ac4, 0x664009b7)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osEepromProbe)


////////////////////////////////////////////////////////////
//        __osEepromRead_Prepare
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osEepromRead_Prepare)
  PATCH_XREF_VAR_HI(1, osEepPifBuffer)
  PATCH_XREF_VAR_LO(2, osEepPifBuffer)
  PATCH_XREF_VAR_HI(8, osEepPifBuffer)
  PATCH_XREF_VAR_LO(12, osEepPifBuffer)
  PATCH_XREF_VAR_HI(18, osEepPifThingamy)
  PATCH_XREF_VAR_LO(22, osEepPifThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osEepromRead_Prepare)
   PATCH_SIGNATURE_LIST_ENTRY(__osEepromRead_Prepare, 65, 9, 0xe4297c94, 0x7050ca49)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osEepromRead_Prepare)


////////////////////////////////////////////////////////////
//       __osEepromWrite_Prepare
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osEepromWrite_Prepare)
  PATCH_XREF_VAR_HI(1, osEepPifBuffer)
  PATCH_XREF_VAR_LO(2, osEepPifBuffer)
  PATCH_XREF_VAR_HI(8, osEepPifBuffer)
  PATCH_XREF_VAR_LO(12, osEepPifBuffer)
  PATCH_XREF_VAR_HI(18, osEepPifThingamy)
  PATCH_XREF_VAR_LO(22, osEepPifThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osEepromWrite_Prepare)
   PATCH_SIGNATURE_LIST_ENTRY(__osEepromWrite_Prepare, 67, 9, 0xe4297c94, 0xaf50420d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osEepromWrite_Prepare)


////////////////////////////////////////////////////////////
//                 __osEepStatus
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osEepStatus)
  PATCH_XREF_VAR_HI(1, osEepPifBuffer)
  PATCH_XREF_VAR_LO(2, osEepPifBuffer)
  PATCH_XREF_VAR_HI(11, osEepPifBuffer)
  PATCH_XREF_VAR_LO(14, osEepPifBuffer)
  PATCH_XREF_VAR_HI(19, osEepPifBuffer)
  PATCH_XREF_VAR_HI(21, osEepPifThingamy)
  PATCH_XREF_VAR_LO(22, osEepPifBuffer)
  PATCH_XREF_VAR_LO(23, osEepPifThingamy)
  PATCH_XREF_VAR_HI(55, osEepPifBuffer)
  PATCH_XREF_VAR_LO(59, osEepPifBuffer)
  PATCH_XREF_FUNCTION(66, __osSiRawStartDma)
  PATCH_XREF_FUNCTION(71, osRecvMesg)
  PATCH_XREF_VAR_HI(74, osEepPifThingamy2)
  PATCH_XREF_VAR_HI(75, osEepPifBuffer)
  PATCH_XREF_VAR_LO(76, osEepPifThingamy2)
  PATCH_XREF_VAR_LO(77, osEepPifBuffer)
  PATCH_XREF_FUNCTION(78, __osSiRawStartDma)
  PATCH_XREF_FUNCTION(83, osRecvMesg)
  PATCH_XREF_VAR_HI(90, osEepPifBuffer)
  PATCH_XREF_VAR_LO(91, osEepPifBuffer)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osEepStatus)
   PATCH_SIGNATURE_LIST_ENTRY(__osEepStatus, 137, 9, 0x3b5e01ec, 0x062d3a0a)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osEepStatus)


////////////////////////////////////////////////////////////
//                         bcopy
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(bcopy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(bcopy)
   PATCH_SIGNATURE_LIST_ENTRY(bcopy, 193, 4, 0xb9b52236, 0x0580e516)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(bcopy)


////////////////////////////////////////////////////////////
//                         bzero
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(bzero)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(bzero)
   PATCH_SIGNATURE_LIST_ENTRY(bzero, 40, 10, 0x9fdfd8da, 0x7c92fb1b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(bzero)


////////////////////////////////////////////////////////////
//                        memcpy
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(memcpy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(memcpy)
   PATCH_SIGNATURE_LIST_ENTRY(memcpy, 11, 0, 0x11e142d4, 0x3512dc6b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(memcpy)


////////////////////////////////////////////////////////////
//                        strlen
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(strlen)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(strlen)
   PATCH_SIGNATURE_LIST_ENTRY(strlen, 10, 36, 0xc6145c7f, 0x8dc53047)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(strlen)


////////////////////////////////////////////////////////////
//                        strchr
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(strchr)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(strchr)
   PATCH_SIGNATURE_LIST_ENTRY(strchr, 16, 36, 0xe8c5827a, 0x9ae8a84a)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(strchr)


////////////////////////////////////////////////////////////
//                        strcmp
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(strcmp)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(strcmp)
   PATCH_SIGNATURE_LIST_ENTRY(strcmp, 24, 21, 0x12dd5fbf, 0x2930e8ce)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(strcmp)


////////////////////////////////////////////////////////////
//       __osSiCreateAccessQueue
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiCreateAccessQueue)
  PATCH_XREF_VAR_HI(3, osSiAccessQueueCreated)
  PATCH_XREF_VAR_HI(4, osSiAccessQueue)
  PATCH_XREF_VAR_HI(5, osSiAccessQueueBuffer)
  PATCH_XREF_VAR_LO(6, osSiAccessQueueCreated)
  PATCH_XREF_VAR_LO(7, osSiAccessQueueBuffer)
  PATCH_XREF_VAR_LO(8, osSiAccessQueue)
  PATCH_XREF_FUNCTION(9, osCreateMesgQueue)
  PATCH_XREF_VAR_HI(11, osSiAccessQueue)
  PATCH_XREF_VAR_LO(12, osSiAccessQueue)
  PATCH_XREF_FUNCTION(14, osSendMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiCreateAccessQueue)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiCreateAccessQueue, 20, 9, 0x087e25a5, 0x3a628950)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiCreateAccessQueue)


////////////////////////////////////////////////////////////
//               __osSiGetAccess
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiGetAccess)
  PATCH_XREF_VAR_HI(0, osSiAccessQueueCreated)
  PATCH_XREF_VAR_LO(1, osSiAccessQueueCreated)
  PATCH_XREF_FUNCTION(6, __osSiCreateAccessQueue)
  PATCH_XREF_VAR_HI(8, osSiAccessQueue)
  PATCH_XREF_VAR_LO(9, osSiAccessQueue)
  PATCH_XREF_FUNCTION(11, osRecvMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiGetAccess)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiGetAccess, 17, 15, 0x7593c323, 0x9cec0b4e)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiGetAccess)


////////////////////////////////////////////////////////////
//               __osSiRelAccess
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiRelAccess)
  PATCH_XREF_VAR_HI(2, osSiAccessQueue)
  PATCH_XREF_VAR_LO(3, osSiAccessQueue)
  PATCH_XREF_FUNCTION(5, osSendMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiRelAccess)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRelAccess, 11, 9, 0x60472e46, 0x9f265393)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiRelAccess)


////////////////////////////////////////////////////////////
//              __osSiDeviceBusy
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiDeviceBusy)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiDeviceBusy, 11, 15, 0xa50073c4, 0x1bd8ff27)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiDeviceBusy)


////////////////////////////////////////////////////////////
//             __osSiRawStartDma
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSiRawStartDma_Mario)
  PATCH_XREF_FUNCTION(3, __osSiDeviceBusy)
  PATCH_XREF_FUNCTION(14, osWritebackDCache)
  PATCH_XREF_FUNCTION(16, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(36, osInvalDCache)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osSiRawStartDma_Rugrats)
  PATCH_XREF_FUNCTION(16, osWritebackDCache)
  PATCH_XREF_FUNCTION(18, osVirtualToPhysical)
  PATCH_XREF_FUNCTION(33, osInvalDCache)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSiRawStartDma)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawStartDma_Mario, 43, 9, 0x2af9bd43, 0x07a24c6f)
   PATCH_SIGNATURE_LIST_ENTRY(__osSiRawStartDma_Rugrats, 41, 9, 0x2063fd1c, 0xa485add6)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSiRawStartDma)


////////////////////////////////////////////////////////////
//           __osPackRequestData
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osPackRequestData)
  PATCH_XREF_VAR_HI(5, osPackRequestDataBuffer)
  PATCH_XREF_VAR_LO(8, osPackRequestDataBuffer)
  PATCH_XREF_VAR_HI(13, osNumControllers)
  PATCH_XREF_VAR_LO(14, osNumControllers)
  PATCH_XREF_VAR_HI(15, osPackRequestDataBuffer)
  PATCH_XREF_VAR_HI(17, osPackRequestDataBufferLastByte)
  PATCH_XREF_VAR_LO(18, osPackRequestDataBuffer)
  PATCH_XREF_VAR_LO(26, osPackRequestDataBufferLastByte)
  PATCH_XREF_VAR_HI(41, osNumControllers)
  PATCH_XREF_VAR_LO(48, osNumControllers)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osPackRequestData)
   PATCH_SIGNATURE_LIST_ENTRY(__osPackRequestData, 61, 9, 0x730f6421, 0x3ea2daf5)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osPackRequestData)


////////////////////////////////////////////////////////////
//           __osContGetInitData
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osContGetInitData)
  PATCH_XREF_VAR_HI(0, osNumControllers)
  PATCH_XREF_VAR_LO(1, osNumControllers)
  PATCH_XREF_VAR_HI(3, osPackRequestDataBuffer)
  PATCH_XREF_VAR_LO(4, osPackRequestDataBuffer)
  PATCH_XREF_VAR_HI(38, osNumControllers)
  PATCH_XREF_VAR_LO(39, osNumControllers)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osContGetInitData)
   PATCH_SIGNATURE_LIST_ENTRY(__osContGetInitData, 52, 15, 0x9ab17da4, 0x626ad2e7)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osContGetInitData)


////////////////////////////////////////////////////////////
//       __osPiCreateAccessQueue
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osPiCreateAccessQueue)
  PATCH_XREF_VAR_HI(3, osPiAccessQueueCreated)
  PATCH_XREF_VAR_HI(4, osPiAccessQueue)
  PATCH_XREF_VAR_HI(5, osPiAccessQueueBuffer)
  PATCH_XREF_VAR_LO(6, osPiAccessQueueCreated)
  PATCH_XREF_VAR_LO(7, osPiAccessQueueBuffer)
  PATCH_XREF_VAR_LO(8, osPiAccessQueue)
  PATCH_XREF_FUNCTION(9, osCreateMesgQueue)
  PATCH_XREF_VAR_HI(11, osPiAccessQueue)
  PATCH_XREF_VAR_LO(12, osPiAccessQueue)
  PATCH_XREF_FUNCTION(14, osSendMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osPiCreateAccessQueue)
   PATCH_SIGNATURE_LIST_ENTRY(__osPiCreateAccessQueue, 20, 9, 0x087e25a5, 0x3a628950)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osPiCreateAccessQueue)


////////////////////////////////////////////////////////////
//               __osPiGetAccess
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osPiGetAccess)
  PATCH_XREF_VAR_HI(0, osPiAccessQueueCreated)
  PATCH_XREF_VAR_LO(1, osPiAccessQueueCreated)
  PATCH_XREF_FUNCTION(6, __osPiCreateAccessQueue)
  PATCH_XREF_VAR_HI(8, osPiAccessQueue)
  PATCH_XREF_VAR_LO(9, osPiAccessQueue)
  PATCH_XREF_FUNCTION(11, osRecvMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osPiGetAccess)
   PATCH_SIGNATURE_LIST_ENTRY(__osPiGetAccess, 17, 15, 0x7593c323, 0x9cec0b4e)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osPiGetAccess)


////////////////////////////////////////////////////////////
//               __osPiRelAccess
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osPiRelAccess)
  PATCH_XREF_VAR_HI(2, osPiAccessQueue)
  PATCH_XREF_VAR_LO(3, osPiAccessQueue)
  PATCH_XREF_FUNCTION(5, osSendMesg)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osPiRelAccess)
   PATCH_SIGNATURE_LIST_ENTRY(__osPiRelAccess, 11, 9, 0x60472e46, 0x9f265393)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osPiRelAccess)


////////////////////////////////////////////////////////////
//             osCreateMesgQueue
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osCreateMesgQueue_Mario)
  PATCH_XREF_VAR_HI(0, osNullMsgQueue)
  PATCH_XREF_VAR_HI(1, osNullMsgQueue)
  PATCH_XREF_VAR_LO(2, osNullMsgQueue)
  PATCH_XREF_VAR_LO(3, osNullMsgQueue)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osCreateMesgQueue_Rugrats)
  PATCH_XREF_VAR_HI(0, osNullMsgQueue)
  PATCH_XREF_VAR_LO(1, osNullMsgQueue)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osCreateMesgQueue)
   PATCH_SIGNATURE_LIST_ENTRY(osCreateMesgQueue_Mario, 12, 15, 0x33ccc69e, 0x5b1bd4ba)
   PATCH_SIGNATURE_LIST_ENTRY(osCreateMesgQueue_Rugrats, 9, 15, 0x6a5c9a07, 0x6dcc8ef6)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osCreateMesgQueue)


////////////////////////////////////////////////////////////
//                    osRecvMesg
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osRecvMesg)
  PATCH_XREF_FUNCTION(6, __osDisableInt)
  PATCH_XREF_FUNCTION(16, __osRestoreInt)
  PATCH_XREF_VAR_HI(20, osActiveThread)
  PATCH_XREF_VAR_LO(21, osActiveThread)
  PATCH_XREF_FUNCTION(24, __osEnqueueAndYield)
  PATCH_XREF_FUNCTION(65, __osPopThread)
  PATCH_XREF_FUNCTION(68, osStartThread)
  PATCH_XREF_FUNCTION(70, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osRecvMesg)
   PATCH_SIGNATURE_LIST_ENTRY(osRecvMesg, 78, 9, 0x2e6b775b, 0xcfe7b388)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osRecvMesg)


////////////////////////////////////////////////////////////
//                    osSendMesg
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSendMesg)
  PATCH_XREF_FUNCTION(7, __osDisableInt)
  PATCH_XREF_VAR_HI(20, osActiveThread)
  PATCH_XREF_VAR_LO(21, osActiveThread)
  PATCH_XREF_FUNCTION(25, __osEnqueueAndYield)
  PATCH_XREF_FUNCTION(29, __osRestoreInt)
  PATCH_XREF_FUNCTION(69, __osPopThread)
  PATCH_XREF_FUNCTION(72, osStartThread)
  PATCH_XREF_FUNCTION(74, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSendMesg)
   PATCH_SIGNATURE_LIST_ENTRY(osSendMesg, 84, 9, 0xf6cb6f8b, 0x3a9dac5b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSendMesg)


////////////////////////////////////////////////////////////
//                osSetEventMesg
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSetEventMesg_Mario)
  PATCH_XREF_FUNCTION(5, __osDisableInt)
  PATCH_XREF_VAR_HI(8, osEventMesgArray)
  PATCH_XREF_VAR_LO(10, osEventMesgArray)
  PATCH_XREF_FUNCTION(19, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osSetEventMesg_Zelda)
  PATCH_XREF_FUNCTION(5, __osDisableInt)
  PATCH_XREF_VAR_HI(8, osEventMesgArray)
  PATCH_XREF_VAR_LO(10, osEventMesgArray)
  PATCH_XREF_VAR_HI(23, osSetMesgEventUnk1)
  PATCH_XREF_VAR_LO(24, osSetMesgEventUnk1)
  PATCH_XREF_VAR_HI(27, osSetMesgEventUnk2)
  PATCH_XREF_VAR_LO(28, osSetMesgEventUnk2)
  PATCH_XREF_FUNCTION(33, osSendMesg)
  PATCH_XREF_VAR_HI(36, osSetMesgEventUnk2)
  PATCH_XREF_VAR_LO(37, osSetMesgEventUnk2)
  PATCH_XREF_FUNCTION(38, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSetEventMesg)
   PATCH_SIGNATURE_LIST_ENTRY(osSetEventMesg_Mario, 26, 9, 0x2e6b775b, 0x4a01363e)
   PATCH_SIGNATURE_LIST_ENTRY(osSetEventMesg_Zelda, 45, 9, 0x2e6b775b, 0x09f01820)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSetEventMesg)


////////////////////////////////////////////////////////////
//            __osDispatchThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osDispatchThread_Mario)
  PATCH_XREF_VAR_HI(0, osThreadQueue)
  PATCH_XREF_FUNCTION(1, __osPopThread)
  PATCH_XREF_VAR_LO(2, osThreadQueue)
  PATCH_XREF_VAR_HI(3, osActiveThread)
  PATCH_XREF_VAR_LO(4, osActiveThread)
  PATCH_XREF_VAR_HI(68, osDispatchThreadRCPThingamy)
  PATCH_XREF_VAR_LO(69, osDispatchThreadRCPThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osDispatchThread_MarioKart)
  PATCH_XREF_VAR_HI(0, osThreadQueue)
  PATCH_XREF_FUNCTION(1, __osPopThread)
  PATCH_XREF_VAR_LO(2, osThreadQueue)
  PATCH_XREF_VAR_HI(3, osActiveThread)
  PATCH_XREF_VAR_LO(4, osActiveThread)
  PATCH_XREF_VAR_HI(8, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(10, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(77, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(78, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(83, osDispatchThreadRCPThingamy)
  PATCH_XREF_VAR_LO(84, osDispatchThreadRCPThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osDispatchThread_Rugrats)
  PATCH_XREF_VAR_HI(0, osThreadQueue)
  PATCH_XREF_FUNCTION(1, __osPopThread)
  PATCH_XREF_VAR_LO(2, osThreadQueue)
  PATCH_XREF_VAR_HI(3, osActiveThread)
  PATCH_XREF_VAR_LO(4, osActiveThread)
  PATCH_XREF_VAR_HI(9, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(10, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(77, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(78, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(83, osDispatchThreadRCPThingamy)
  PATCH_XREF_VAR_LO(84, osDispatchThreadRCPThingamy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osDispatchThread)
   PATCH_SIGNATURE_LIST_ENTRY(__osDispatchThread_Mario, 80, 15, 0x09f4f142, 0x66c5ab1a)
   PATCH_SIGNATURE_LIST_ENTRY(__osDispatchThread_MarioKart, 95, 15, 0x09f4f142, 0x1d77a71e)
   PATCH_SIGNATURE_LIST_ENTRY(__osDispatchThread_Rugrats, 95, 15, 0x09f4f142, 0xb8c90e75)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osDispatchThread)


////////////////////////////////////////////////////////////
//             __osDequeueThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osDequeueThread)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osDequeueThread)
   PATCH_SIGNATURE_LIST_ENTRY(__osDequeueThread, 16, 0, 0x9372f012, 0xa238df35)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osDequeueThread)


////////////////////////////////////////////////////////////
//                osCreateThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osCreateThread_Mario)
  PATCH_XREF_VAR_HI(28, osThreadDieRA)
  PATCH_XREF_VAR_LO(37, osThreadDieRA)
  PATCH_XREF_FUNCTION(64, __osDisableInt)
  PATCH_XREF_VAR_HI(66, osGlobalThreadList)
  PATCH_XREF_VAR_LO(67, osGlobalThreadList)
  PATCH_XREF_VAR_HI(70, osGlobalThreadList)
  PATCH_XREF_FUNCTION(74, __osRestoreInt)
  PATCH_XREF_VAR_LO(75, osGlobalThreadList)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osCreateThread_Rugrats)
  PATCH_XREF_VAR_HI(5, osThreadDieRA)
  PATCH_XREF_VAR_LO(6, osThreadDieRA)
  PATCH_XREF_FUNCTION(39, __osDisableInt)
  PATCH_XREF_VAR_HI(41, osGlobalThreadList)
  PATCH_XREF_VAR_LO(42, osGlobalThreadList)
  PATCH_XREF_VAR_HI(44, osGlobalThreadList)
  PATCH_XREF_VAR_LO(45, osGlobalThreadList)
  PATCH_XREF_FUNCTION(46, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osCreateThread)
   PATCH_SIGNATURE_LIST_ENTRY(osCreateThread_Mario, 81, 9, 0x71862463, 0x40a11ef6)
   PATCH_SIGNATURE_LIST_ENTRY(osCreateThread_Rugrats, 52, 9, 0x95e59b96, 0x8a7f2916)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osCreateThread)


////////////////////////////////////////////////////////////
//                osSetThreadPri
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSetThreadPri)
  PATCH_XREF_FUNCTION(4, __osDisableInt)
  PATCH_XREF_VAR_HI(10, osActiveThread)
  PATCH_XREF_VAR_LO(11, osActiveThread)
  PATCH_XREF_VAR_HI(19, osActiveThread)
  PATCH_XREF_VAR_LO(20, osActiveThread)
  PATCH_XREF_FUNCTION(29, __osDequeueThread)
  PATCH_XREF_FUNCTION(33, __osEnqueueThread)
  PATCH_XREF_VAR_HI(35, osActiveThread)
  PATCH_XREF_VAR_HI(36, osThreadQueue)
  PATCH_XREF_VAR_LO(37, osThreadQueue)
  PATCH_XREF_VAR_LO(38, osActiveThread)
  PATCH_XREF_VAR_HI(45, osThreadQueue)
  PATCH_XREF_FUNCTION(47, __osEnqueueAndYield)
  PATCH_XREF_VAR_LO(48, osThreadQueue)
  PATCH_XREF_FUNCTION(49, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSetThreadPri)
   PATCH_SIGNATURE_LIST_ENTRY(osSetThreadPri, 56, 9, 0x2e6b775b, 0x33cf992c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSetThreadPri)


////////////////////////////////////////////////////////////
//                osGetThreadPri
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osGetThreadPri)
  PATCH_XREF_VAR_HI(2, osActiveThread)
  PATCH_XREF_VAR_LO(3, osActiveThread)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osGetThreadPri)
   PATCH_SIGNATURE_LIST_ENTRY(osGetThreadPri, 6, 5, 0x761f59dc, 0xe6176d0d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osGetThreadPri)


////////////////////////////////////////////////////////////
//           __osEnqueueAndYield
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osEnqueueAndYield_Mario)
  PATCH_XREF_VAR_HI(0, osActiveThread)
  PATCH_XREF_VAR_LO(1, osActiveThread)
  PATCH_XREF_FUNCTION(32, __osEnqueueThread)
  PATCH_XREF_FUNCTION(34, __osDispatchThread)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osEnqueueAndYield_MarioKart)
  PATCH_XREF_VAR_HI(0, osActiveThread)
  PATCH_XREF_VAR_LO(1, osActiveThread)
  PATCH_XREF_VAR_HI(32, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(33, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(48, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(49, osInterruptMaskThingy)
  PATCH_XREF_FUNCTION(60, __osEnqueueThread)
  PATCH_XREF_FUNCTION(62, __osDispatchThread)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osEnqueueAndYield)
   PATCH_SIGNATURE_LIST_ENTRY(__osEnqueueAndYield_Mario, 36, 15, 0x734f12eb, 0xc584a2f8)
   PATCH_SIGNATURE_LIST_ENTRY(__osEnqueueAndYield_MarioKart, 64, 15, 0x734f12eb, 0xc494d817)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osEnqueueAndYield)


////////////////////////////////////////////////////////////
//                 __osPopThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osPopThread)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osPopThread)
   PATCH_SIGNATURE_LIST_ENTRY(__osPopThread, 4, 35, 0x8cf40658, 0x8cf40658)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osPopThread)


////////////////////////////////////////////////////////////
//             __osEnqueueThread
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osEnqueueThread_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osEnqueueThread_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osEnqueueThread)
   PATCH_SIGNATURE_LIST_ENTRY(__osEnqueueThread_Mario, 17, 35, 0xe7f50e17, 0x2a950d35)
   PATCH_SIGNATURE_LIST_ENTRY(__osEnqueueThread_Rugrats, 18, 0, 0xbb87e666, 0x27ab7389)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osEnqueueThread)


////////////////////////////////////////////////////////////
//                     osSetTime
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osSetTime)
  PATCH_XREF_VAR_HI(3, osSystemTimeHi)
  PATCH_XREF_VAR_LO(5, osSystemTimeHi)
  PATCH_XREF_VAR_HI(6, osSystemTimeLo)
  PATCH_XREF_VAR_LO(8, osSystemTimeLo)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osSetTime)
   PATCH_SIGNATURE_LIST_ENTRY(osSetTime, 9, 43, 0x8881d679, 0xacd31b7b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osSetTime)


////////////////////////////////////////////////////////////
//                     osGetTime
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osGetTime)
  PATCH_XREF_FUNCTION(2, __osDisableInt)
  PATCH_XREF_FUNCTION(4, osGetCount)
  PATCH_XREF_VAR_HI(7, osSystemCount)
  PATCH_XREF_VAR_LO(8, osSystemCount)
  PATCH_XREF_VAR_HI(10, osSystemTimeHi)
  PATCH_XREF_VAR_HI(11, osSystemTimeLo)
  PATCH_XREF_VAR_LO(12, osSystemTimeLo)
  PATCH_XREF_VAR_LO(13, osSystemTimeHi)
  PATCH_XREF_FUNCTION(18, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osGetTime)
   PATCH_SIGNATURE_LIST_ENTRY(osGetTime, 33, 9, 0x1f290f7a, 0x5c27ab62)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osGetTime)


////////////////////////////////////////////////////////////
//              __osSetTimerIntr
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSetTimerIntr)
  PATCH_XREF_FUNCTION(3, __osDisableInt)
  PATCH_XREF_FUNCTION(5, osGetCount)
  PATCH_XREF_VAR_HI(7, osSystemLastCount)
  PATCH_XREF_VAR_LO(8, osSystemLastCount)
  PATCH_XREF_VAR_HI(9, osSystemLastCount)
  PATCH_XREF_VAR_LO(10, osSystemLastCount)
  PATCH_XREF_FUNCTION(21, __osSetCompare)
  PATCH_XREF_FUNCTION(23, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSetTimerIntr)
   PATCH_SIGNATURE_LIST_ENTRY(__osSetTimerIntr, 29, 9, 0x5e2cabdd, 0x3cb15718)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSetTimerIntr)


////////////////////////////////////////////////////////////
//           osVirtualToPhysical
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osVirtualToPhysical_Mario)
  PATCH_XREF_FUNCTION(25, __osProbeTLB)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osVirtualToPhysical_Rugrats)
  PATCH_XREF_FUNCTION(13, __osProbeTLB)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osVirtualToPhysical)
   PATCH_SIGNATURE_LIST_ENTRY(osVirtualToPhysical_Mario, 31, 9, 0x4e109cd2, 0x5fcc4093)
   PATCH_SIGNATURE_LIST_ENTRY(osVirtualToPhysical_Rugrats, 21, 9, 0x0de0280f, 0xb9943e6c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osVirtualToPhysical)


////////////////////////////////////////////////////////////
//                  __osProbeTLB
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osProbeTLB)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osProbeTLB)
   PATCH_SIGNATURE_LIST_ENTRY(__osProbeTLB, 46, 16, 0x8279ed61, 0x55ab04a3)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osProbeTLB)


////////////////////////////////////////////////////////////
//                   osMapTLBRdb
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osMapTLBRdb)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osMapTLBRdb)
   PATCH_SIGNATURE_LIST_ENTRY(osMapTLBRdb, 22, 16, 0xab3f79c8, 0xe7e58c9c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osMapTLBRdb)


////////////////////////////////////////////////////////////
//                      osMapTLB
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osMapTLB)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osMapTLB)
   PATCH_SIGNATURE_LIST_ENTRY(osMapTLB, 45, 16, 0xeaef7986, 0x694fd4c3)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osMapTLB)


////////////////////////////////////////////////////////////
//                 osUnmapTLBAll
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osUnmapTLBAll_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osUnmapTLBAll_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osUnmapTLBAll)
   PATCH_SIGNATURE_LIST_ENTRY(osUnmapTLBAll_Mario, 17, 16, 0x272229e0, 0x4560a499)
   PATCH_SIGNATURE_LIST_ENTRY(osUnmapTLBAll_Rugrats, 17, 16, 0xbc87658f, 0x6f10c364)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osUnmapTLBAll)


////////////////////////////////////////////////////////////
//          osWritebackDCacheAll
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osWritebackDCacheAll)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osWritebackDCacheAll)
   PATCH_SIGNATURE_LIST_ENTRY(osWritebackDCacheAll, 10, 15, 0xab7a075b, 0xf462714e)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osWritebackDCacheAll)


////////////////////////////////////////////////////////////
//             osWritebackDCache
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osWritebackDCache_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osWritebackDCache_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osWritebackDCache)
   PATCH_SIGNATURE_LIST_ENTRY(osWritebackDCache_Mario, 29, 6, 0xa10f5ae9, 0x3575328f)
   PATCH_SIGNATURE_LIST_ENTRY(osWritebackDCache_Rugrats, 29, 6, 0xa10f5ae9, 0x153cd45b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osWritebackDCache)


////////////////////////////////////////////////////////////
//                 osInvalDCache
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osInvalDCache_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osInvalDCache_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osInvalDCache)
   PATCH_SIGNATURE_LIST_ENTRY(osInvalDCache_Mario, 43, 6, 0x818dac42, 0x6b4a048a)
   PATCH_SIGNATURE_LIST_ENTRY(osInvalDCache_Rugrats, 44, 6, 0xfe7f8e9b, 0x83faa077)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osInvalDCache)


////////////////////////////////////////////////////////////
//                 osInvalICache
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osInvalICache_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(osInvalICache_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osInvalICache)
   PATCH_SIGNATURE_LIST_ENTRY(osInvalICache_Mario, 29, 6, 0x2ab9160c, 0xc660b068)
   PATCH_SIGNATURE_LIST_ENTRY(osInvalICache_Rugrats, 29, 6, 0x2ab9160c, 0xe62956bc)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osInvalICache)


////////////////////////////////////////////////////////////
//                  osViSetEvent
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osViSetEvent)
  PATCH_XREF_FUNCTION(5, __osDisableInt)
  PATCH_XREF_VAR_HI(7, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(8, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(10, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(11, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(13, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(18, osViSetModeGubbins)
  PATCH_XREF_FUNCTION(20, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osViSetEvent)
   PATCH_SIGNATURE_LIST_ENTRY(osViSetEvent, 27, 9, 0x2e6b775b, 0x874ca31b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osViSetEvent)


////////////////////////////////////////////////////////////
//                   osViSetMode
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osViSetMode)
  PATCH_XREF_FUNCTION(3, __osDisableInt)
  PATCH_XREF_VAR_HI(5, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(6, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(8, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(11, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(12, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(15, osViSetModeGubbins)
  PATCH_XREF_FUNCTION(19, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osViSetMode)
   PATCH_SIGNATURE_LIST_ENTRY(osViSetMode, 26, 9, 0x5820dd23, 0xea2b91fb)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osViSetMode)


////////////////////////////////////////////////////////////
//                     osViBlack
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osViBlack)
  PATCH_XREF_FUNCTION(3, __osDisableInt)
  PATCH_XREF_VAR_HI(9, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(10, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(15, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(16, osViSetModeGubbins)
  PATCH_XREF_FUNCTION(21, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osViBlack)
   PATCH_SIGNATURE_LIST_ENTRY(osViBlack, 28, 9, 0x5820dd23, 0x2615a092)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osViBlack)


////////////////////////////////////////////////////////////
//                osViSwapBuffer
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osViSwapBuffer)
  PATCH_XREF_FUNCTION(2, __osDisableInt)
  PATCH_XREF_VAR_HI(4, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(5, osViSetModeGubbins)
  PATCH_XREF_VAR_HI(8, osViSetModeGubbins)
  PATCH_XREF_VAR_LO(10, osViSetModeGubbins)
  PATCH_XREF_FUNCTION(14, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osViSwapBuffer)
   PATCH_SIGNATURE_LIST_ENTRY(osViSwapBuffer, 20, 9, 0x66389aad, 0xd116472a)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osViSwapBuffer)


////////////////////////////////////////////////////////////
//                 osAiGetLength
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osAiGetLength)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osAiGetLength)
   PATCH_SIGNATURE_LIST_ENTRY(osAiGetLength, 4, 15, 0xa7fbab0f, 0xa7fbab0f)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osAiGetLength)


////////////////////////////////////////////////////////////
//              __osAiDeviceBusy
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osAiDeviceBusy)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osAiDeviceBusy)
   PATCH_SIGNATURE_LIST_ENTRY(__osAiDeviceBusy, 12, 15, 0xf3702e8a, 0xd49d742f)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osAiDeviceBusy)


////////////////////////////////////////////////////////////
//             osAiSetNextBuffer
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osAiSetNextBuffer)
  PATCH_XREF_VAR_HI(1, osAiSetNextBufferThingy)
  PATCH_XREF_VAR_LO(2, osAiSetNextBufferThingy)
  PATCH_XREF_VAR_HI(19, osAiSetNextBufferThingy)
  PATCH_XREF_VAR_LO(21, osAiSetNextBufferThingy)
  PATCH_XREF_VAR_HI(22, osAiSetNextBufferThingy)
  PATCH_XREF_VAR_LO(23, osAiSetNextBufferThingy)
  PATCH_XREF_FUNCTION(24, __osAiDeviceBusy)
  PATCH_XREF_FUNCTION(30, osVirtualToPhysical)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osAiSetNextBuffer)
   PATCH_SIGNATURE_LIST_ENTRY(osAiSetNextBuffer, 42, 9, 0xda4cf31a, 0x682588aa)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osAiSetNextBuffer)


////////////////////////////////////////////////////////////
//                  __osGetCause
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osGetCause)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osGetCause)
   PATCH_SIGNATURE_LIST_ENTRY(__osGetCause, 3, 16, 0xc9a418cf, 0xc9a418cf)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osGetCause)


////////////////////////////////////////////////////////////
//                __osDisableInt
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osDisableInt_Mario)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osDisableInt_Zelda)
  PATCH_XREF_VAR_HI(0, osInterruptMaskThingy)
  PATCH_XREF_VAR_LO(1, osInterruptMaskThingy)
  PATCH_XREF_VAR_HI(12, osActiveThread)
  PATCH_XREF_VAR_LO(13, osActiveThread)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osDisableInt)
   PATCH_SIGNATURE_LIST_ENTRY(__osDisableInt_Mario, 8, 16, 0x8fa7930f, 0xc2e1ff4b)
   PATCH_SIGNATURE_LIST_ENTRY(__osDisableInt_Zelda, 28, 15, 0xa7179da8, 0xcee25777)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osDisableInt)


////////////////////////////////////////////////////////////
//                __osRestoreInt
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osRestoreInt)
   PATCH_SIGNATURE_LIST_ENTRY(__osRestoreInt, 7, 16, 0x49bd020c, 0xad8bfd1d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osRestoreInt)


////////////////////////////////////////////////////////////
//                 __osAtomicDec
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osAtomicDec)
  PATCH_XREF_FUNCTION(2, __osDisableInt)
  PATCH_XREF_FUNCTION(15, __osRestoreInt)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osAtomicDec)
   PATCH_SIGNATURE_LIST_ENTRY(__osAtomicDec, 22, 9, 0x66389aad, 0xc470beb2)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osAtomicDec)


////////////////////////////////////////////////////////////
//                 __osSetFpcCsr
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSetFpcCsr)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSetFpcCsr)
   PATCH_SIGNATURE_LIST_ENTRY(__osSetFpcCsr, 4, 17, 0xf52165ba, 0xf52165ba)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSetFpcCsr)


////////////////////////////////////////////////////////////
//                __osSetCompare
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSetCompare)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSetCompare)
   PATCH_SIGNATURE_LIST_ENTRY(__osSetCompare, 3, 16, 0x0ad52a95, 0x0ad52a95)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSetCompare)


////////////////////////////////////////////////////////////
//                    osGetCount
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osGetCount)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osGetCount)
   PATCH_SIGNATURE_LIST_ENTRY(osGetCount, 3, 16, 0x4f389718, 0x4f389718)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osGetCount)


////////////////////////////////////////////////////////////
//                     __osSetSR
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osSetSR)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osSetSR)
   PATCH_SIGNATURE_LIST_ENTRY(__osSetSR, 4, 16, 0x0bd68503, 0x0bd68503)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osSetSR)


////////////////////////////////////////////////////////////
//                     __osGetSR
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osGetSR)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osGetSR)
   PATCH_SIGNATURE_LIST_ENTRY(__osGetSR, 3, 16, 0x7367f98a, 0x7367f98a)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osGetSR)


////////////////////////////////////////////////////////////
//                  __ull_rshift
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ull_rshift)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ull_rshift)
   PATCH_SIGNATURE_LIST_ENTRY(__ull_rshift, 11, 43, 0xfb20959a, 0xd53fbeaa)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ull_rshift)


////////////////////////////////////////////////////////////
//                     __ull_div
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ull_div)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ull_div)
   PATCH_SIGNATURE_LIST_ENTRY(__ull_div, 15, 43, 0xfb20959a, 0x1fc4e322)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ull_div)


////////////////////////////////////////////////////////////
//                   __ll_lshift
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ll_lshift)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ll_lshift)
   PATCH_SIGNATURE_LIST_ENTRY(__ll_lshift, 11, 43, 0xfb20959a, 0x1ff1c826)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ll_lshift)


////////////////////////////////////////////////////////////
//                     __ull_rem
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ull_rem)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ull_rem)
   PATCH_SIGNATURE_LIST_ENTRY(__ull_rem, 15, 43, 0xfb20959a, 0xd50a95ae)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ull_rem)


////////////////////////////////////////////////////////////
//                      __ll_div
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ll_div)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ll_div)
   PATCH_SIGNATURE_LIST_ENTRY(__ll_div, 23, 43, 0xfb20959a, 0xf4f7fb03)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ll_div)


////////////////////////////////////////////////////////////
//                      __ull_mul
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ull_mul)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ull_mul)
   PATCH_SIGNATURE_LIST_ENTRY(__ull_mul, 12, 43, 0xfb20959a, 0x2e2b7b73)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ull_mul)


////////////////////////////////////////////////////////////
//                 __ull_divremi
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ull_divremi)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ull_divremi)
   PATCH_SIGNATURE_LIST_ENTRY(__ull_divremi, 24, 33, 0x936c5bed, 0x12b381a8)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ull_divremi)


////////////////////////////////////////////////////////////
//                      __ll_mod
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ll_mod)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ll_mod)
   PATCH_SIGNATURE_LIST_ENTRY(__ll_mod, 39, 9, 0xceb40b69, 0x224b83e0)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ll_mod)


////////////////////////////////////////////////////////////
//                   __ll_rshift
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ll_rshift)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ll_rshift)
   PATCH_SIGNATURE_LIST_ENTRY(__ll_rshift, 11, 43, 0xfb20959a, 0xb05885ec)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ll_rshift)


////////////////////////////////////////////////////////////
//                       __lldiv
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__lldiv)
  PATCH_XREF_FUNCTION(8, __ll_div)
  PATCH_XREF_FUNCTION(15, __ull_mul)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__lldiv)
   PATCH_SIGNATURE_LIST_ENTRY(__lldiv, 64, 9, 0xd0282313, 0x4a9550af)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__lldiv)


////////////////////////////////////////////////////////////
//                        __ldiv
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__ldiv)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__ldiv)
   PATCH_SIGNATURE_LIST_ENTRY(__ldiv, 33, 0, 0x160bd90b, 0x7e26414c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__ldiv)


////////////////////////////////////////////////////////////
//         __osTimerServicesInit
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(__osTimerServicesInit_Mario)
  PATCH_XREF_VAR_HI(0, osSystemTimeHi)
  PATCH_XREF_VAR_LO(3, osSystemTimeLo)
  PATCH_XREF_VAR_LO(4, osSystemTimeHi)
  PATCH_XREF_VAR_HI(5, osSystemCount)
  PATCH_XREF_VAR_HI(6, osTopTimer)
  PATCH_XREF_VAR_LO(7, osTopTimer)
  PATCH_XREF_VAR_LO(8, osSystemCount)
  PATCH_XREF_VAR_HI(9, osFrameCount)
  PATCH_XREF_VAR_LO(10, osFrameCount)
  PATCH_XREF_VAR_HI(11, osTopTimer)
  PATCH_XREF_VAR_LO(13, osTopTimer)
  PATCH_XREF_VAR_HI(14, osTopTimer)
  PATCH_XREF_VAR_HI(18, osTopTimer)
  PATCH_XREF_VAR_LO(20, osTopTimer)
  PATCH_XREF_VAR_HI(21, osTopTimer)
  PATCH_XREF_VAR_HI(22, osTopTimer)
  PATCH_XREF_VAR_LO(25, osTopTimer)
  PATCH_XREF_VAR_LO(30, osTopTimer) // buggy, breaks Killer Instinct
  PATCH_XREF_VAR_LO(32, osTopTimer)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(__osTimerServicesInit_Rugrats)
 PATCH_XREF_VAR_HI(0, osTopTimer)
  PATCH_XREF_VAR_LO(1, osTopTimer)
  PATCH_XREF_VAR_HI(4, osSystemTimeHi)
  PATCH_XREF_VAR_LO(5, osSystemTimeHi)
  PATCH_XREF_VAR_HI(6, osSystemTimeLo)
  PATCH_XREF_VAR_LO(7, osSystemTimeLo)
  PATCH_XREF_VAR_HI(8, osSystemCount)
  PATCH_XREF_VAR_LO(9, osSystemCount)
  PATCH_XREF_VAR_HI(10, osFrameCount)
  PATCH_XREF_VAR_LO(11, osFrameCount)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(__osTimerServicesInit)
   PATCH_SIGNATURE_LIST_ENTRY(__osTimerServicesInit_Mario, 35, 15, 0x25c8f154, 0x02794387)
   PATCH_SIGNATURE_LIST_ENTRY(__osTimerServicesInit_Rugrats, 21, 15, 0x5e897bbd, 0x7be63a06)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(__osTimerServicesInit)


////////////////////////////////////////////////////////////
//                     guRotateF
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guRotateF)
  PATCH_XREF_VAR_HI(1, osRotateFUnk1)
  PATCH_XREF_VAR_LO(2, osRotateFUnk1)
  PATCH_XREF_VAR_HI(5, osRotateFUnk2)
  PATCH_XREF_FUNCTION(13, guNormalize)
  PATCH_XREF_VAR_LO(14, osRotateFUnk2)
  PATCH_XREF_VAR_HI(15, osRotateFUnk2)
  PATCH_XREF_VAR_LO(17, osRotateFUnk2)
  PATCH_XREF_FUNCTION(19, sinf)
  PATCH_XREF_FUNCTION(22, cosf)
  PATCH_XREF_FUNCTION(42, guMtxIdentF)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guRotateF)
   PATCH_SIGNATURE_LIST_ENTRY(guRotateF, 101, 9, 0x30448e4c, 0xb78e753d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guRotateF)


////////////////////////////////////////////////////////////
//                  guTranslateF
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guTranslateF)
  PATCH_XREF_FUNCTION(5, guMtxIdentF)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guTranslateF)
   PATCH_SIGNATURE_LIST_ENTRY(guTranslateF, 18, 9, 0x725f261f, 0xb9b1614a)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guTranslateF)


////////////////////////////////////////////////////////////
//                   guTranslate
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guTranslate)
  PATCH_XREF_FUNCTION(6, guMtxIdentF)
  PATCH_XREF_FUNCTION(15, guMtxF2L)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guTranslate)
   PATCH_SIGNATURE_LIST_ENTRY(guTranslate, 21, 9, 0xb4906d8f, 0xb7aae154)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guTranslate)


////////////////////////////////////////////////////////////
//                      guScaleF
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guScaleF)
  PATCH_XREF_FUNCTION(5, guMtxIdentF)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guScaleF)
   PATCH_SIGNATURE_LIST_ENTRY(guScaleF, 21, 9, 0x725f261f, 0x702c46fc)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guScaleF)


////////////////////////////////////////////////////////////
//                       guScale
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guScale)
  PATCH_XREF_FUNCTION(9, guScaleF)
  PATCH_XREF_FUNCTION(12, guMtxF2L)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guScale)
   PATCH_SIGNATURE_LIST_ENTRY(guScale, 18, 17, 0xa6d05d38, 0xb2b9e45d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guScale)


////////////////////////////////////////////////////////////
//                   guMtxIdentF
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guMtxIdentF)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guMtxIdentF)
   PATCH_SIGNATURE_LIST_ENTRY(guMtxIdentF, 34, 15, 0x748c766e, 0x33a242c6)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guMtxIdentF)


////////////////////////////////////////////////////////////
//                    guMtxIdent
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guMtxIdent)
  PATCH_XREF_FUNCTION(3, guMtxIdentF)
  PATCH_XREF_FUNCTION(6, guMtxF2L)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guMtxIdent)
   PATCH_SIGNATURE_LIST_ENTRY(guMtxIdent, 12, 9, 0x2d1a8954, 0x85a7c28b)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guMtxIdent)


////////////////////////////////////////////////////////////
//                   guNormalize
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guNormalize_Mario)
	PATCH_XREF_FUNCTION(12, sqrtf)
END_PATCH_XREFS()
BEGIN_PATCH_XREFS(guNormalize_Rugrats)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guNormalize)
   PATCH_SIGNATURE_LIST_ENTRY(guNormalize_Mario, 33, 9, 0x1f8bec4f, 0x2211764d)
   PATCH_SIGNATURE_LIST_ENTRY(guNormalize_Rugrats, 18, 49, 0x5b383db0, 0x54f835c8)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guNormalize)


////////////////////////////////////////////////////////////
//                      guMtxF2L
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guMtxF2L)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guMtxF2L)
   PATCH_SIGNATURE_LIST_ENTRY(guMtxF2L, 64, 15, 0xbd861287, 0xfa43fa1d)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guMtxF2L)


////////////////////////////////////////////////////////////
//                         sqrtf
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(sqrtf)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(sqrtf)
   PATCH_SIGNATURE_LIST_ENTRY(sqrtf, 2, 0, 0xbf291d5c, 0xbf291d5c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(sqrtf)


////////////////////////////////////////////////////////////
//                          sinf
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(sinf)
  PATCH_XREF_VAR_HI(12, osSinfUnk1)
  PATCH_XREF_VAR_LO(13, osSinfUnk1)
  PATCH_XREF_VAR_HI(36, osSinfUnk2)
  PATCH_XREF_VAR_LO(37, osSinfUnk2)
  PATCH_XREF_VAR_HI(64, osSinfUnk3)
  PATCH_XREF_VAR_LO(65, osSinfUnk3)
  PATCH_XREF_VAR_HI(67, osSinfUnk4)
  PATCH_XREF_VAR_LO(68, osSinfUnk4)
  PATCH_XREF_VAR_HI(69, osSinfUnk1)
  PATCH_XREF_VAR_LO(70, osSinfUnk1)
  PATCH_XREF_VAR_HI(103, osSinfUnk5)
  PATCH_XREF_VAR_HI(106, osSinfUnk6)
  PATCH_XREF_VAR_LO(108, osSinfUnk6)
  PATCH_XREF_VAR_LO(109, osSinfUnk5)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(sinf)
   PATCH_SIGNATURE_LIST_ENTRY(sinf, 112, 57, 0x4f6e8d66, 0x6108ee5c)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(sinf)


////////////////////////////////////////////////////////////
//                          cosf
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(cosf)
  PATCH_XREF_VAR_HI(14, osCosfUnk1)
  PATCH_XREF_VAR_LO(20, osCosfUnk1)
  PATCH_XREF_VAR_HI(41, osCosfUnk2)
  PATCH_XREF_VAR_LO(42, osCosfUnk2)
  PATCH_XREF_VAR_HI(44, osCosfUnk3)
  PATCH_XREF_VAR_LO(45, osCosfUnk3)
  PATCH_XREF_VAR_HI(46, osCosfUnk4)
  PATCH_XREF_VAR_LO(47, osCosfUnk4)
  PATCH_XREF_VAR_HI(81, osCosfUnk5)
  PATCH_XREF_VAR_HI(84, osCosfUnk6)
  PATCH_XREF_VAR_LO(86, osCosfUnk6)
  PATCH_XREF_VAR_LO(87, osCosfUnk5)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(cosf)
   PATCH_SIGNATURE_LIST_ENTRY(cosf, 90, 57, 0x8a6a6469, 0x0c20b1cf)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(cosf)


////////////////////////////////////////////////////////////
//                      guOrthoF
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guOrthoF)
  PATCH_XREF_FUNCTION(5, guMtxIdentF)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guOrthoF)
   PATCH_SIGNATURE_LIST_ENTRY(guOrthoF, 85, 9, 0x725f261f, 0x4b90e514)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guOrthoF)


////////////////////////////////////////////////////////////
//                       guOrtho
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(guOrtho)
  PATCH_XREF_FUNCTION(17, guOrthoF)
  PATCH_XREF_FUNCTION(20, guMtxF2L)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(guOrtho)
   PATCH_SIGNATURE_LIST_ENTRY(guOrtho, 26, 9, 0xd64137ce, 0x0cca6627)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(guOrtho)

////////////////////////////////////////////////////////////
//               osPiRawStartDma
////////////////////////////////////////////////////////////
BEGIN_PATCH_XREFS(osPiRawStartDma)
  PATCH_XREF_FUNCTION(18, osVirtualToPhysical)
END_PATCH_XREFS()
BEGIN_PATCH_SIGNATURE_LIST(osPiRawStartDma)
   PATCH_SIGNATURE_LIST_ENTRY(osPiRawStartDma, 56, 9, 0x2e6b775b, 0x631ca293)
END_PATCH_SIGNATURE_LIST()

PATCH_SYMBOL_FUNCTION(osPiRawStartDma)

PATCH_SYMBOL_VARIABLE(osSystemTimeHi)
PATCH_SYMBOL_VARIABLE(osSystemTimeLo)
PATCH_SYMBOL_VARIABLE(osSystemCount)
PATCH_SYMBOL_VARIABLE(osFrameCount)
PATCH_SYMBOL_VARIABLE(osSystemLastCount)
PATCH_SYMBOL_VARIABLE(osNullMsgQueue)
PATCH_SYMBOL_VARIABLE(osThreadQueue)
PATCH_SYMBOL_VARIABLE(osActiveThread)
PATCH_SYMBOL_VARIABLE(osEventMesgArray)
PATCH_SYMBOL_VARIABLE(osThreadDieRA)
PATCH_SYMBOL_VARIABLE(osGlobalThreadList)
PATCH_SYMBOL_VARIABLE(osDispatchThreadRCPThingamy)
PATCH_SYMBOL_VARIABLE(osInterruptMaskThingy)
PATCH_SYMBOL_VARIABLE(osEepPifBuffer)
PATCH_SYMBOL_VARIABLE(osEepPifThingamy)
PATCH_SYMBOL_VARIABLE(osEepPifThingamy2)
PATCH_SYMBOL_VARIABLE(osAiSetNextBufferThingy)
PATCH_SYMBOL_VARIABLE(osPiAccessQueueCreated)
PATCH_SYMBOL_VARIABLE(osPiAccessQueueBuffer)
PATCH_SYMBOL_VARIABLE(osPiAccessQueue)
PATCH_SYMBOL_VARIABLE(osSiAccessQueueCreated)
PATCH_SYMBOL_VARIABLE(osSiAccessQueueBuffer)
PATCH_SYMBOL_VARIABLE(osSiAccessQueue)
PATCH_SYMBOL_VARIABLE(osSpTaskLoadTempTask)
PATCH_SYMBOL_VARIABLE(osViSetModeGubbins)
PATCH_SYMBOL_VARIABLE(osPackRequestDataBuffer)
PATCH_SYMBOL_VARIABLE(osPackRequestDataBufferLastByte)
PATCH_SYMBOL_VARIABLE(osNumControllers)
PATCH_SYMBOL_VARIABLE(osContInitialised)
PATCH_SYMBOL_VARIABLE(osClockRateHi)
PATCH_SYMBOL_VARIABLE(osClockRateLo)
PATCH_SYMBOL_VARIABLE(osContInitUnk)
PATCH_SYMBOL_VARIABLE(osEepromMQ)
PATCH_SYMBOL_VARIABLE(osEepromMsgBuffer)
PATCH_SYMBOL_VARIABLE(osTopTimer)
PATCH_SYMBOL_VARIABLE(osCosfUnk1)
PATCH_SYMBOL_VARIABLE(osCosfUnk2)
PATCH_SYMBOL_VARIABLE(osCosfUnk3)
PATCH_SYMBOL_VARIABLE(osCosfUnk4)
PATCH_SYMBOL_VARIABLE(osCosfUnk5)
PATCH_SYMBOL_VARIABLE(osCosfUnk6)
PATCH_SYMBOL_VARIABLE(osSinfUnk1)
PATCH_SYMBOL_VARIABLE(osSinfUnk2)
PATCH_SYMBOL_VARIABLE(osSinfUnk3)
PATCH_SYMBOL_VARIABLE(osSinfUnk4)
PATCH_SYMBOL_VARIABLE(osSinfUnk5)
PATCH_SYMBOL_VARIABLE(osSinfUnk6)
PATCH_SYMBOL_VARIABLE(osRotateFUnk1)
PATCH_SYMBOL_VARIABLE(osRotateFUnk2)
PATCH_SYMBOL_VARIABLE(osSetMesgEventUnk1)
PATCH_SYMBOL_VARIABLE(osSetMesgEventUnk2)
BEGIN_PATCH_SYMBOL_TABLE(g_PatchSymbols)

#ifndef DISABLE_THREAD_FUNCS
PATCH_FUNCTION_ENTRY(osStartThread)
PATCH_FUNCTION_ENTRY(osDestroyThread)
PATCH_FUNCTION_ENTRY(__osDispatchThread)
PATCH_FUNCTION_ENTRY(__osDequeueThread)
PATCH_FUNCTION_ENTRY(osCreateThread)
//PATCH_FUNCTION_ENTRY(osSetThreadPri)
PATCH_FUNCTION_ENTRY(osGetThreadPri)
PATCH_FUNCTION_ENTRY(__osEnqueueAndYield)
PATCH_FUNCTION_ENTRY(__osPopThread)
PATCH_FUNCTION_ENTRY(__osEnqueueThread)
#endif

#ifndef DISABLE_SP_FUNCTIONS
PATCH_FUNCTION_ENTRY(osSpTaskStartGo) //buggy, SSB (?? Seems to work fine -Salvy)
//PATCH_FUNCTION_ENTRY(osSpTaskYield)
//PATCH_FUNCTION_ENTRY(osSpTaskYielded)
PATCH_FUNCTION_ENTRY(osSpTaskLoad)
//PATCH_FUNCTION_ENTRY(__osSpTaskLoadInitTask)
PATCH_FUNCTION_ENTRY(__osSpRawStartDma)
PATCH_FUNCTION_ENTRY(__osSpSetStatus)
PATCH_FUNCTION_ENTRY(__osSpGetStatus)
PATCH_FUNCTION_ENTRY(__osSpSetPc)
PATCH_FUNCTION_ENTRY(__osSpDeviceBusy)
#endif

#ifndef DISABLE_IOFUNCS
PATCH_FUNCTION_ENTRY(__osSiRawReadIo)
PATCH_FUNCTION_ENTRY(__osSiRawWriteIo)
//PATCH_FUNCTION_ENTRY(osEepromRead)
//PATCH_FUNCTION_ENTRY(osEepromWrite)
PATCH_FUNCTION_ENTRY(osEepromProbe)
//PATCH_FUNCTION_ENTRY(__osEepromRead_Prepare)
//PATCH_FUNCTION_ENTRY(__osEepromWrite_Prepare)
PATCH_FUNCTION_ENTRY(__osEepStatus)
PATCH_FUNCTION_ENTRY(__osSiCreateAccessQueue)
PATCH_FUNCTION_ENTRY(__osSiGetAccess)
PATCH_FUNCTION_ENTRY(__osSiRelAccess)
PATCH_FUNCTION_ENTRY(__osSiDeviceBusy)
PATCH_FUNCTION_ENTRY(__osSiRawStartDma)
//PATCH_FUNCTION_ENTRY(__osPackRequestData)
//PATCH_FUNCTION_ENTRY(__osContGetInitData)
PATCH_FUNCTION_ENTRY(__osPiCreateAccessQueue)
PATCH_FUNCTION_ENTRY(__osPiGetAccess)
PATCH_FUNCTION_ENTRY(__osPiRelAccess)
PATCH_FUNCTION_ENTRY(osVirtualToPhysical)
PATCH_FUNCTION_ENTRY(__osProbeTLB)
//PATCH_FUNCTION_ENTRY(osMapTLBRdb)
//PATCH_FUNCTION_ENTRY(osMapTLB)
//PATCH_FUNCTION_ENTRY(osUnmapTLBAll)
PATCH_FUNCTION_ENTRY(osWritebackDCacheAll)
PATCH_FUNCTION_ENTRY(osWritebackDCache)
PATCH_FUNCTION_ENTRY(osInvalDCache)
PATCH_FUNCTION_ENTRY(osInvalICache)
//PATCH_FUNCTION_ENTRY(osViSetEvent)
//PATCH_FUNCTION_ENTRY(osViSetMode)
//PATCH_FUNCTION_ENTRY(osViBlack)
PATCH_FUNCTION_ENTRY(osViSwapBuffer)
PATCH_FUNCTION_ENTRY(osAiGetLength)
PATCH_FUNCTION_ENTRY(__osAiDeviceBusy)
PATCH_FUNCTION_ENTRY(osAiSetNextBuffer)
PATCH_FUNCTION_ENTRY(osPiRawStartDma)
#endif
#ifndef DISABLE_MSG_FUNCTIONS
PATCH_FUNCTION_ENTRY(osCreateMesgQueue)
PATCH_FUNCTION_ENTRY(osRecvMesg)
PATCH_FUNCTION_ENTRY(osSendMesg)
PATCH_FUNCTION_ENTRY(osSetEventMesg)
#endif
#ifndef DISABLE_CPU_FUNCTIONS
PATCH_FUNCTION_ENTRY(__osGetCause)
PATCH_FUNCTION_ENTRY(__osDisableInt)
PATCH_FUNCTION_ENTRY(__osRestoreInt)  //buggy, breaks DOOM64
PATCH_FUNCTION_ENTRY(__osAtomicDec)
PATCH_FUNCTION_ENTRY(__osSetFpcCsr)
PATCH_FUNCTION_ENTRY(__osSetCompare)
PATCH_FUNCTION_ENTRY(osGetCount)
PATCH_FUNCTION_ENTRY(__osSetSR)
PATCH_FUNCTION_ENTRY(__osGetSR)
//PATCH_FUNCTION_ENTRY(osContInit)
//PATCH_FUNCTION_ENTRY(osSetIntMask)
#endif
#ifndef DISABLE_TIMER_FUNCTIONS
PATCH_FUNCTION_ENTRY(__osTimerServicesInit)
PATCH_FUNCTION_ENTRY(osSetTime)
PATCH_FUNCTION_ENTRY(osGetTime)
PATCH_FUNCTION_ENTRY(__osSetTimerIntr)
//PATCH_FUNCTION_ENTRY(osSetTimer)
PATCH_FUNCTION_ENTRY(__osInsertTimer)
#endif
#ifndef DISABLE_CSTDFUNCS
PATCH_FUNCTION_ENTRY(bcopy)
PATCH_FUNCTION_ENTRY(bzero)
PATCH_FUNCTION_ENTRY(memcpy)
PATCH_FUNCTION_ENTRY(strlen)
PATCH_FUNCTION_ENTRY(strchr)
PATCH_FUNCTION_ENTRY(strcmp)
#endif
#ifndef DISABLE_LLFUNCS
PATCH_FUNCTION_ENTRY(__ull_rshift)
PATCH_FUNCTION_ENTRY(__ull_div)
PATCH_FUNCTION_ENTRY(__ll_lshift)
PATCH_FUNCTION_ENTRY(__ull_rem)
PATCH_FUNCTION_ENTRY(__ll_div)
PATCH_FUNCTION_ENTRY(__ull_mul)
//PATCH_FUNCTION_ENTRY(__ull_divremi) // Is not implemented anyways
PATCH_FUNCTION_ENTRY(__ll_mod)
PATCH_FUNCTION_ENTRY(__ll_rshift)
//PATCH_FUNCTION_ENTRY(__lldiv)
//PATCH_FUNCTION_ENTRY(__ldiv)
#endif


#ifndef DISABLE_GUFUNCS
PATCH_FUNCTION_ENTRY(guRotateF)
PATCH_FUNCTION_ENTRY(guTranslateF)
PATCH_FUNCTION_ENTRY(guTranslate)
PATCH_FUNCTION_ENTRY(guScaleF)
PATCH_FUNCTION_ENTRY(guScale)
PATCH_FUNCTION_ENTRY(guMtxIdentF)
PATCH_FUNCTION_ENTRY(guMtxIdent)
PATCH_FUNCTION_ENTRY(guNormalize)
PATCH_FUNCTION_ENTRY(guMtxF2L)
PATCH_FUNCTION_ENTRY(sqrtf)
PATCH_FUNCTION_ENTRY(sinf)
PATCH_FUNCTION_ENTRY(cosf)
PATCH_FUNCTION_ENTRY(guOrthoF)
PATCH_FUNCTION_ENTRY(guOrtho)
#endif

END_PATCH_SYMBOL_TABLE()

BEGIN_PATCH_VARIABLE_TABLE(g_PatchVariables)
PATCH_VARIABLE_ENTRY(osSystemTimeHi)
PATCH_VARIABLE_ENTRY(osSystemTimeLo)
PATCH_VARIABLE_ENTRY(osSystemCount)
PATCH_VARIABLE_ENTRY(osFrameCount)
PATCH_VARIABLE_ENTRY(osSystemLastCount)
PATCH_VARIABLE_ENTRY(osNullMsgQueue)
PATCH_VARIABLE_ENTRY(osThreadQueue)
PATCH_VARIABLE_ENTRY(osActiveThread)
PATCH_VARIABLE_ENTRY(osEventMesgArray)
PATCH_VARIABLE_ENTRY(osThreadDieRA)
PATCH_VARIABLE_ENTRY(osGlobalThreadList)
PATCH_VARIABLE_ENTRY(osDispatchThreadRCPThingamy)
PATCH_VARIABLE_ENTRY(osInterruptMaskThingy)
PATCH_VARIABLE_ENTRY(osEepPifBuffer)
PATCH_VARIABLE_ENTRY(osEepPifThingamy)
PATCH_VARIABLE_ENTRY(osEepPifThingamy2)
PATCH_VARIABLE_ENTRY(osAiSetNextBufferThingy)
PATCH_VARIABLE_ENTRY(osPiAccessQueueCreated)
PATCH_VARIABLE_ENTRY(osPiAccessQueueBuffer)
PATCH_VARIABLE_ENTRY(osPiAccessQueue)
PATCH_VARIABLE_ENTRY(osSiAccessQueueCreated)
PATCH_VARIABLE_ENTRY(osSiAccessQueueBuffer)
PATCH_VARIABLE_ENTRY(osSiAccessQueue)
PATCH_VARIABLE_ENTRY(osSpTaskLoadTempTask)
PATCH_VARIABLE_ENTRY(osViSetModeGubbins)
PATCH_VARIABLE_ENTRY(osPackRequestDataBuffer)
PATCH_VARIABLE_ENTRY(osPackRequestDataBufferLastByte)
PATCH_VARIABLE_ENTRY(osNumControllers)
PATCH_VARIABLE_ENTRY(osContInitialised)
PATCH_VARIABLE_ENTRY(osClockRateHi)
PATCH_VARIABLE_ENTRY(osClockRateLo)
PATCH_VARIABLE_ENTRY(osContInitUnk)
PATCH_VARIABLE_ENTRY(osEepromMQ)
PATCH_VARIABLE_ENTRY(osEepromMsgBuffer)
PATCH_VARIABLE_ENTRY(osTopTimer)
PATCH_VARIABLE_ENTRY(osCosfUnk1)
PATCH_VARIABLE_ENTRY(osCosfUnk2)
PATCH_VARIABLE_ENTRY(osCosfUnk3)
PATCH_VARIABLE_ENTRY(osCosfUnk4)
PATCH_VARIABLE_ENTRY(osCosfUnk5)
PATCH_VARIABLE_ENTRY(osCosfUnk6)
PATCH_VARIABLE_ENTRY(osSinfUnk1)
PATCH_VARIABLE_ENTRY(osSinfUnk2)
PATCH_VARIABLE_ENTRY(osSinfUnk3)
PATCH_VARIABLE_ENTRY(osSinfUnk4)
PATCH_VARIABLE_ENTRY(osSinfUnk5)
PATCH_VARIABLE_ENTRY(osSinfUnk6)
PATCH_VARIABLE_ENTRY(osRotateFUnk1)
PATCH_VARIABLE_ENTRY(osRotateFUnk2)
PATCH_VARIABLE_ENTRY(osSetMesgEventUnk1)
PATCH_VARIABLE_ENTRY(osSetMesgEventUnk2)
END_PATCH_VARIABLE_TABLE()

