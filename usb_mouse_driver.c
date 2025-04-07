#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/hid.h>

#define USB_MOUSE_VENDOR_ID  0x046D  // Logitech
#define USB_MOUSE_PRODUCT_ID 0xC077  // M105 Optical Mouse

struct usb_mouse {
    struct input_dev *input;
    struct usb_device *udev;
    struct urb *irq;
    unsigned char *data;
    dma_addr_t data_dma;
};

static void usb_mouse_irq(struct urb *urb)
{
    struct usb_mouse *mouse = urb->context;
    signed char *data = mouse->data;

    if (urb->status)
        return;

    input_report_key(mouse->input, BTN_LEFT, data[0] & 0x01);
    input_report_key(mouse->input, BTN_RIGHT, data[0] & 0x02);
    input_report_key(mouse->input, BTN_MIDDLE, data[0] & 0x04);
    input_report_rel(mouse->input, REL_X, data[1]);
    input_report_rel(mouse->input, REL_Y, data[2]);
    input_sync(mouse->input);

    usb_submit_urb(urb, GFP_ATOMIC);
}

static int usb_mouse_probe(struct usb_interface *iface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(iface);
    struct usb_host_interface *interface = iface->cur_altsetting;
    struct usb_endpoint_descriptor *endpoint;
    struct usb_mouse *mouse;
    int pipe, maxp;

    if (interface->desc.bNumEndpoints < 1)
        return -ENODEV;

    endpoint = &interface->endpoint[0].desc;
    if (!usb_endpoint_is_int_in(endpoint))
        return -ENODEV;

    mouse = kzalloc(sizeof(struct usb_mouse), GFP_KERNEL);
    if (!mouse)
        return -ENOMEM;

    mouse->udev = udev;
    mouse->input = input_allocate_device();
    if (!mouse->input) {
        kfree(mouse);
        return -ENOMEM;
    }

    input_set_capability(mouse->input, EV_KEY, BTN_LEFT);
    input_set_capability(mouse->input, EV_KEY, BTN_RIGHT);
    input_set_capability(mouse->input, EV_KEY, BTN_MIDDLE);
    input_set_capability(mouse->input, EV_REL, REL_X);
    input_set_capability(mouse->input, EV_REL, REL_Y);
    
    if (input_register_device(mouse->input)) {
        input_free_device(mouse->input);
        kfree(mouse);
        return -ENOMEM;
    }

    mouse->irq = usb_alloc_urb(0, GFP_KERNEL);
    if (!mouse->irq) {
        input_unregister_device(mouse->input);
        input_free_device(mouse->input);
        kfree(mouse);
        return -ENOMEM;
    }

    mouse->data = usb_alloc_coherent(udev, 8, GFP_ATOMIC, &mouse->data_dma);
    if (!mouse->data) {
        usb_free_urb(mouse->irq);
        input_unregister_device(mouse->input);
        input_free_device(mouse->input);
        kfree(mouse);
        return -ENOMEM;
    }

    pipe = usb_rcvintpipe(udev, endpoint->bEndpointAddress);
    maxp = usb_maxpacket(udev, pipe);
    usb_fill_int_urb(mouse->irq, udev, pipe, mouse->data, maxp, usb_mouse_irq, mouse, endpoint->bInterval);
    mouse->irq->transfer_dma = mouse->data_dma;
    mouse->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
    usb_set_intfdata(iface, mouse);
    usb_submit_urb(mouse->irq, GFP_KERNEL);
    
    return 0;
}

static void usb_mouse_disconnect(struct usb_interface *iface)
{
    struct usb_mouse *mouse = usb_get_intfdata(iface);

    usb_kill_urb(mouse->irq);
    input_unregister_device(mouse->input);
    usb_free_urb(mouse->irq);
    usb_free_coherent(mouse->udev, 8, mouse->data, mouse->data_dma);
    kfree(mouse);
}

static const struct usb_device_id usb_mouse_id_table[] = {
    { USB_DEVICE(USB_MOUSE_VENDOR_ID, USB_MOUSE_PRODUCT_ID) },
    { }
};
MODULE_DEVICE_TABLE(usb, usb_mouse_id_table);

static struct usb_driver usb_mouse_driver = {
    .name = "usb_mouse_logitech_m105",
    .probe = usb_mouse_probe,
    .disconnect = usb_mouse_disconnect,
    .id_table = usb_mouse_id_table,
};

module_usb_driver(usb_mouse_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("USB Driver for Logitech M105 Optical Mouse");
