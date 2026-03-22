void panic(const char *msg)
{
    // print msg using printk
    while (1)
    {
        __asm__ volatile("hlt");
    }
}