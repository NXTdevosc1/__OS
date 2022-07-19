[Defines]
  PLATFORM_NAME                  = EfiBoot
  PLATFORM_GUID                  = e0a78d1a-5c95-4e48-954d-88ff64af9ab1
  PLATFORM_VERSION               = 1.0
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/EfiBoot
  SUPPORTED_ARCHITECTURES        = IA32|X64|EBC|ARM|AARCH64|RISCV64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT


[Components]
  EfiBoot/bootx64.inf