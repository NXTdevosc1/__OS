// CreatorID=INTL	CreatorRev=20.10.1320
// FileLength=7344	FileChkSum=0x7

DefinitionBlock("q35-acpi-dsdt.aml", "DSDT", 0x01, "BXPC", "BXDSDT", 0x00000002)
{
    Scope(\)
    {
        OperationRegion(DBG_, SystemIO, 0x402, One)
        Field(DBG_, ByteAcc, NoLock, Preserve)
        {
            DBGB, 8
        }
        Method(DBUG, 0x1, NotSerialized)
        {
            ToHexString(Arg0, Local0)
            ToBuffer(Local0, Local0)
            Subtract(SizeOf(Local0), One, Local1)
            Store(Zero, Local2)
            While(LLess(Local2, Local1))
            {
                Store(DerefOf(Index(Local0, Local2, )), DBGB)
                Increment(Local2)
            }
            Store(0xa, DBGB)
        }
    }
    Scope(_SB_)
    {
        OperationRegion(PCST, SystemIO, 0xae00, 0xc)
        OperationRegion(PCSB, SystemIO, 0xae0c, One)
        Field(PCSB, AnyAcc, NoLock, WriteAsZeros)
        {
            PCIB, 8
        }
    }
    Scope(_SB_)
    {
        Device(PCI0)
        {
            Name(_HID, 0x80ad041)
            Name(_CID, 0x30ad041)
            Name(_ADR, Zero)
            Name(_UID, One)
            Name(SUPP, Zero)
            Name(CTRL, Zero)
            Method(_OSC, 0x4, NotSerialized)
            {
                CreateDWordField(Arg3, Zero, CDW1)
                If(LEqual(Arg0, Buffer(0x10)
                {
	0x5b, 0x4d, 0xdb, 0x33, 0xf7, 0x1f, 0x1c, 0x40, 0x96, 0x57, 0x74, 0x41,
	0xc0, 0x3d, 0xd7, 0x66
                }))
                {
                    CreateDWordField(Arg3, 0x4, CDW2)
                    CreateDWordField(Arg3, 0x8, CDW3)
                    Store(CDW2, SUPP)
                    Store(CDW3, CTRL)
                    And(CTRL, 0x1d, CTRL)
                    If(LNot(LEqual(Arg1, One)))
                    {
                        Or(CDW1, 0x8, CDW1)
                    }
                    If(LNot(LEqual(CDW3, CTRL)))
                    {
                        Or(CDW1, 0x10, CDW1)
                    }
                    Store(CTRL, CDW3)
                }
                Else
                {
                    Or(CDW1, 0x4, CDW1)
                }
                Return(Arg3)
            }
        }
    }
    Scope(_SB_.PCI0)
    {
        Name(CRES, Buffer(0x6e)
        {
	0x88, 0x0d, 0x00, 0x02, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x47, 0x01, 0xf8, 0x0c, 0xf8, 0x0c, 0x01, 0x08,
	0x88, 0x0d, 0x00, 0x01, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0xf7, 0x0c,
	0x00, 0x00, 0xf8, 0x0c, 0x88, 0x0d, 0x00, 0x01, 0x0c, 0x03, 0x00, 0x00,
	0x00, 0x0d, 0xff, 0xff, 0x00, 0x00, 0x00, 0xf3, 0x87, 0x17, 0x00, 0x00,
	0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0xff, 0xff,
	0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x87, 0x17,
	0x00, 0x00, 0x0c, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0,
	0xff, 0xff, 0xbf, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x1e,
	0x79, 0x00
        })
        Name(CR64, Buffer(0x30)
        {
	0x8a, 0x2b, 0x00, 0x00, 0x0c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x79, 0x00
        })
        Method(_CRS, 0x0, NotSerialized)
        {
            CreateDWordField(CRES, 0x5c, PS32)
            CreateDWordField(CRES, 0x60, PE32)
            CreateDWordField(CRES, 0x68, PL32)
            Store(P0S_, PS32)
            Store(P0E_, PE32)
            Store(Add(Subtract(P0E_, P0S_, ), One, ), PL32)
            If(LEqual(P1V_, Zero))
            {
                Return(CRES)
            }
            CreateQWordField(CR64, 0xe, PS64)
            CreateQWordField(CR64, 0x16, PE64)
            CreateQWordField(CR64, 0x26, PL64)
            Store(P1S_, PS64)
            Store(P1E_, PE64)
            Store(P1L_, PL64)
            ConcatenateResTemplate(CRES, CR64, Local0)
            Return(Local0)
        }
    }
    Scope(_SB_)
    {
        Device(HPET)
        {
            Name(_HID, 0x301d041)
            Name(_UID, Zero)
            OperationRegion(HPTM, SystemMemory, 0xfed00000, 0x400)
            Field(HPTM, DWordAcc, Lock, Preserve)
            {
                VEND, 32,
                PRD_, 32
            }
            Method(_STA, 0x0, NotSerialized)
            {
                Store(VEND, Local0)
                Store(PRD_, Local1)
                ShiftRight(Local0, 0x10, Local0)
                If(LOr(LEqual(Local0, Zero), LEqual(Local0, 0xffff)))
                {
                    Return(Zero)
                }
                If(LOr(LEqual(Local1, Zero), LGreater(Local1, 0x5f5e100)))
                {
                    Return(Zero)
                }
                Return(0xf)
            }
            Name(_CRS, Buffer(0xe)
            {
	0x86, 0x09, 0x00, 0x00, 0x00, 0x00, 0xd0, 0xfe, 0x00, 0x04, 0x00, 0x00,
	0x79, 0x00
            })
        }
    }
    Scope(_SB_.PCI0)
    {
        Device(VGA_)
        {
            Name(_ADR, 0x10000)
            Method(_S1D, 0x0, NotSerialized)
            {
                Return(Zero)
            }
            Method(_S2D, 0x0, NotSerialized)
            {
                Return(Zero)
            }
            Method(_S3D, 0x0, NotSerialized)
            {
                Return(Zero)
            }
        }
    }
    Scope(_SB_.PCI0)
    {
        Device(ISA_)
        {
            Name(_ADR, 0x1f0000)
            OperationRegion(PIRQ, PCI_Config, 0x60, 0xc)
            OperationRegion(LPCD, PCI_Config, 0x80, 0x2)
            Field(LPCD, AnyAcc, NoLock, Preserve)
            {
                COMA, 3,
                , 1,
                COMB, 3,
                , 1,
                LPTD, 2,
                , 2,
                FDCD, 2
            }
            OperationRegion(LPCE, PCI_Config, 0x82, 0x2)
            Field(LPCE, AnyAcc, NoLock, Preserve)
            {
                CAEN, 1,
                CBEN, 1,
                LPEN, 1,
                FDEN, 1
            }
        }
    }
    Scope(_SB_.PCI0.ISA_)
    {
        Device(RTC_)
        {
            Name(_HID, 0xbd041)
            Name(_CRS, Buffer(0x15)
            {
	0x47, 0x01, 0x70, 0x00, 0x70, 0x00, 0x10, 0x02, 0x22, 0x00, 0x01, 0x47,
	0x01, 0x72, 0x00, 0x72, 0x00, 0x02, 0x06, 0x79, 0x00
            })
        }
        Device(KBD_)
        {
            Name(_HID, 0x303d041)
            Method(_STA, 0x0, NotSerialized)
            {
                Return(0xf)
            }
            Name(_CRS, Buffer(0x15)
            {
	0x47, 0x01, 0x60, 0x00, 0x60, 0x00, 0x01, 0x01, 0x47, 0x01, 0x64, 0x00,
	0x64, 0x00, 0x01, 0x01, 0x22, 0x02, 0x00, 0x79, 0x00
            })
        }
        Device(MOU_)
        {
            Name(_HID, 0x130fd041)
            Method(_STA, 0x0, NotSerialized)
            {
                Return(0xf)
            }
            Name(_CRS, Buffer(0x5)
            {
	0x22, 0x00, 0x10, 0x79, 0x00
            })
        }
        Device(FDC0)
        {
            Name(_HID, 0x7d041)
            Method(_STA, 0x0, NotSerialized)
            {
                Store(FDEN, Local0)
                If(LEqual(Local0, Zero))
                {
                    Return(Zero)
                }
                Else
                {
                    Return(0xf)
                }
            }
            Name(_CRS, Buffer(0x18)
            {
	0x47, 0x01, 0xf2, 0x03, 0xf2, 0x03, 0x00, 0x04, 0x47, 0x01, 0xf7, 0x03,
	0xf7, 0x03, 0x00, 0x01, 0x22, 0x40, 0x00, 0x2a, 0x04, 0x00, 0x79, 0x00
            })
        }
        Device(LPT_)
        {
            Name(_HID, 0x4d041)
            Method(_STA, 0x0, NotSerialized)
            {
                Store(LPEN, Local0)
                If(LEqual(Local0, Zero))
                {
                    Return(Zero)
                }
                Else
                {
                    Return(0xf)
                }
            }
            Name(_CRS, Buffer(0xd)
            {
	0x47, 0x01, 0x78, 0x03, 0x78, 0x03, 0x08, 0x08, 0x22, 0x80, 0x00, 0x79,
	0x00
            })
        }
        Device(COM1)
        {
            Name(_HID, 0x105d041)
            Name(_UID, One)
            Method(_STA, 0x0, NotSerialized)
            {
                Store(CAEN, Local0)
                If(LEqual(Local0, Zero))
                {
                    Return(Zero)
                }
                Else
                {
                    Return(0xf)
                }
            }
            Name(_CRS, Buffer(0xd)
            {
	0x47, 0x01, 0xf8, 0x03, 0xf8, 0x03, 0x00, 0x08, 0x22, 0x10, 0x00, 0x79,
	0x00
            })
        }
        Device(COM2)
        {
            Name(_HID, 0x105d041)
            Name(_UID, 0x2)
            Method(_STA, 0x0, NotSerialized)
            {
                Store(CBEN, Local0)
                If(LEqual(Local0, Zero))
                {
                    Return(Zero)
                }
                Else
                {
                    Return(0xf)
                }
            }
            Name(_CRS, Buffer(0xd)
            {
	0x47, 0x01, 0xf8, 0x02, 0xf8, 0x02, 0x00, 0x08, 0x22, 0x08, 0x00, 0x79,
	0x00
            })
        }
    }
    Name(PICF, Zero)
    Method(_PIC, 0x1, NotSerialized)
    {
        Store(Arg0, PICF)
    }
    Scope(_SB_)
    {
        Scope(PCI0)
        {
            Name(PRTP, Package(0x80)
            {
                Package(0x4)
                {
                    0xffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    Zero,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    One,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    0x2,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    0x3,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    Zero,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    One,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    0x2,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    0x3,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    Zero,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    One,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    0x2,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    0x3,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    0x3,
                    LNKD,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    0x3,
                    LNKD,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    0x3,
                    LNKD,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    0x3,
                    LNKD,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    0x3,
                    LNKD,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    Zero,
                    LNKE,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    One,
                    LNKF,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    0x2,
                    LNKG,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    0x3,
                    LNKH,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    Zero,
                    LNKA,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    One,
                    LNKB,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    0x2,
                    LNKC,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    0x3,
                    LNKD,
                    Zero
                }
            })
            Name(PRTA, Package(0x80)
            {
                Package(0x4)
                {
                    0xffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x1ffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x2ffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x3ffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x4ffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x5ffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x6ffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x7ffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x8ffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x9ffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xaffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xbffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xcffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xdffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xeffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0xfffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x10ffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x11ffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x12ffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x13ffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x14ffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    Zero,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    One,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    0x2,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x15ffff,
                    0x3,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    Zero,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    One,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    0x2,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x16ffff,
                    0x3,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    Zero,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    One,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    0x2,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x17ffff,
                    0x3,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x18ffff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x19ffff,
                    0x3,
                    GSID,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x1affff,
                    0x3,
                    GSID,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x1bffff,
                    0x3,
                    GSID,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x1cffff,
                    0x3,
                    GSID,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x1dffff,
                    0x3,
                    GSID,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    Zero,
                    GSIE,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    One,
                    GSIF,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    0x2,
                    GSIG,
                    Zero
                },
                Package(0x4)
                {
                    0x1effff,
                    0x3,
                    GSIH,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    Zero,
                    GSIA,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    One,
                    GSIB,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    0x2,
                    GSIC,
                    Zero
                },
                Package(0x4)
                {
                    0x1fffff,
                    0x3,
                    GSID,
                    Zero
                }
            })
            Method(_PRT, 0x0, NotSerialized)
            {
                If(LEqual(PICF, Zero))
                {
                    Return(PRTP)
                }
                Else
                {
                    Return(PRTA)
                }
            }
        }
        Field(PCI0.ISA_.PIRQ, ByteAcc, NoLock, Preserve)
        {
            PRQA, 8,
            PRQB, 8,
            PRQC, 8,
            PRQD, 8,
            , 32,
            PRQE, 8,
            PRQF, 8,
            PRQG, 8,
            PRQH, 8
        }
        Method(IQST, 0x1, NotSerialized)
        {
            If(And(0x80, Arg0, ))
            {
                Return(0x9)
            }
            Return(0xb)
        }
        Method(IQCR, 0x1, NotSerialized)
        {
            Name(PRR0, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            CreateDWordField(PRR0, 0x5, PRRI)
            Store(And(Arg0, 0xf, ), PRRI)
            Return(PRR0)
        }
        Device(LNKA)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQA))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQA, 0x80, PRQA)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQA))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQA)
            }
        }
        Device(LNKB)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, One)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQB))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQB, 0x80, PRQB)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQB))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQB)
            }
        }
        Device(LNKC)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x2)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQC))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQC, 0x80, PRQC)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQC))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQC)
            }
        }
        Device(LNKD)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x3)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQD))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQD, 0x80, PRQD)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQD))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQD)
            }
        }
        Device(LNKE)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x4)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQE))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQE, 0x80, PRQE)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQE))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQE)
            }
        }
        Device(LNKF)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x5)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQF))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQF, 0x80, PRQF)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQF))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQF)
            }
        }
        Device(LNKG)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x6)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQG))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQG, 0x80, PRQG)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQG))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQG)
            }
        }
        Device(LNKH)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, 0x7)
            Name(_PRS, Buffer(0x13)
            {
	0x89, 0x0e, 0x00, 0x09, 0x03, 0x05, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00,
	0x00, 0x0b, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_STA, 0x0, NotSerialized)
            {
                Return(IQST(PRQH))
            }
            Method(_DIS, 0x0, NotSerialized)
            {
                Or(PRQH, 0x80, PRQH)
            }
            Method(_CRS, 0x0, NotSerialized)
            {
                Return(IQCR(PRQH))
            }
            Method(_SRS, 0x1, NotSerialized)
            {
                CreateDWordField(Arg0, 0x5, PRRI)
                Store(PRRI, PRQH)
            }
        }
        Device(GSIA)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x10, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x10, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIB)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x11, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x11, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIC)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x12, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x12, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSID)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x13, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x13, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIE)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x14, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x14, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIF)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x15, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x15, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIG)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x16, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x16, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
        Device(GSIH)
        {
            Name(_HID, 0xf0cd041)
            Name(_UID, Zero)
            Name(_PRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x17, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Name(_CRS, Buffer(0xb)
            {
	0x89, 0x06, 0x00, 0x09, 0x01, 0x17, 0x00, 0x00, 0x00, 0x79, 0x00
            })
            Method(_SRS, 0x1, NotSerialized)
            {
            }
        }
    }
    Scope(_SB_)
    {
        Method(CPMA, 0x1, NotSerialized)
        {
            Store(DerefOf(Index(CPON, Arg0, )), Local0)
            Store(Buffer(0x8)
            {
	0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
            }, Local1)
            Store(Arg0, Index(Local1, 0x2, ))
            Store(Arg0, Index(Local1, 0x3, ))
            Store(Local0, Index(Local1, 0x4, ))
            Return(Local1)
        }
        Method(CPST, 0x1, NotSerialized)
        {
            Store(DerefOf(Index(CPON, Arg0, )), Local0)
            If(Local0)
            {
                Return(0xf)
            }
            Else
            {
                Return(Zero)
            }
        }
        Method(CPEJ, 0x2, NotSerialized)
        {
            Sleep(0xc8)
        }
        OperationRegion(PRST, SystemIO, 0xaf00, 0x20)
        Field(PRST, ByteAcc, NoLock, Preserve)
        {
            PRS_, 256
        }
        Method(PRSC, 0x0, NotSerialized)
        {
            Store(PRS_, Local5)
            Store(Zero, Local2)
            Store(Zero, Local0)
            While(LLess(Local0, SizeOf(CPON)))
            {
                Store(DerefOf(Index(CPON, Local0, )), Local1)
                If(And(Local0, 0x7, ))
                {
                    ShiftRight(Local2, One, Local2)
                }
                Else
                {
                    Store(DerefOf(Index(Local5, ShiftRight(Local0, 0x3, ), )), Local2)
                }
                Store(And(Local2, One, ), Local3)
                If(LNot(LEqual(Local1, Local3)))
                {
                    Store(Local3, Index(CPON, Local0, ))
                    If(LEqual(Local3, One))
                    {
                        NTFY
                        Local0
                        One
                    }
                    Else
                    {
                        NTFY
                        Local0
                        0x3
                    }
                }
                Increment(Local0)
            }
        }
    }
    Scope(_GPE)
    {
        Name(_HID, "ACPI0006")
        Method(_L00, 0x0, NotSerialized)
        {
        }
        Method(_L01, 0x0, NotSerialized)
        {
            \_SB_.PRSC()
        }
        Method(_L02, 0x0, NotSerialized)
        {
        }
        Method(_L03, 0x0, NotSerialized)
        {
        }
        Method(_L04, 0x0, NotSerialized)
        {
        }
        Method(_L05, 0x0, NotSerialized)
        {
        }
        Method(_L06, 0x0, NotSerialized)
        {
        }
        Method(_L07, 0x0, NotSerialized)
        {
        }
        Method(_L08, 0x0, NotSerialized)
        {
        }
        Method(_L09, 0x0, NotSerialized)
        {
        }
        Method(_L0A, 0x0, NotSerialized)
        {
        }
        Method(_L0B, 0x0, NotSerialized)
        {
        }
        Method(_L0C, 0x0, NotSerialized)
        {
        }
        Method(_L0D, 0x0, NotSerialized)
        {
        }
        Method(_L0E, 0x0, NotSerialized)
        {
        }
        Method(_L0F, 0x0, NotSerialized)
        {
        }
    }
}
