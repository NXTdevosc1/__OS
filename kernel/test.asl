
DefinitionBlock("out.aml", "DSDT", 1, "OEMID", "TABLEID", 1) {
    Scope (\_SB) {
        Device(PCI0) {
            Name(_HID, EisaId("PNP0A03"))
            Name(_ADR, 0x00)
            Name(_UID, 1)
        }
    }
}

