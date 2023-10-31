SECTIONS
{
  caldata :
  {
    /*
    * Dot `.` symbol denotes location counter.
    * It automatically updates with location address.
    * We use it to track the boundaries of the section.
    */
    CalPageStart = .;
    *(caldata)
    CalPageEnd = . - 1;
  }
  xcp_slave_section_address_definitions :
  {
    TextStart = ADDR(.text);
    TextEnd = ADDR(.text) + SIZEOF(.text) - 1;
    BssStart = ADDR(.bss);
    BssEnd = ADDR(.bss) + SIZEOF(.bss) - 1;
    DataStart = ADDR(.data);
    DataEnd = ADDR(.data) + SIZEOF(.data) - 1;
  }
}
INSERT AFTER .data1;
