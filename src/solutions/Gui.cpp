#include "Gui.h"

int VxWindow::onKeyEvent(vx_layer_t* layer, vx_key_event_t* event) {
    return 0;
}

int VxWindow::onMouseEvent(vx_layer_t* layer, vx_camera_pos_t* cameraPosition, vx_mouse_event_t* event) {
    return 0;
}

int VxWindow::setup() {

	//Get Camera url
	zarray_t *urls = image_source_enumerate ();
    char *img_url;
 	if (0==zarray_size (urls)) return -1;
    //Change the 0 to 1 to change to the overhead cameras
	zarray_get (urls, 1, &img_url);

    // Set up the imagesource
    isrc = image_source_open (img_url);

    if (isrc == NULL) return -2;
    else {
        // Print out possible formats. If no format was specified in the
        // url, then format 0 is picked by default.
        // e.g. of setting the format parameter to format 2:
        //
        // --url=dc1394://bd91098db0as9?fidx=2
        for (int i = 0; i < isrc->num_formats (isrc); i++) {
            image_source_format_t ifmt;
            isrc->get_format (isrc, i, &ifmt);
            // printf ("%3d: %4d x %4d (%s)\n",
            //         i, ifmt.width, ifmt.height, ifmt.format);
        }
        isrc->start (isrc);
    }

    pthread_t april_tag_pid;
    pthread_create(&april_tag_pid, NULL, VxWindow::april_thread, NULL);

    //Correctly set the image source
    return 0;
}

VxWindow::Handler::Handler() {}

void VxWindow::Handler::handleApril(const lcm::ReceiveBuffer* rbuf,
                            const std::string& chan,
                            const april_tag_t* msg) {

    // if (hash_tag.find(msg->id)!=hash_tag.end()) {
    //     hash_tag[msg->id] = *msg;
    // } else {
    //     hash_tag[msg->id].push_back(*msg);
    // }
    cout << "ID: " << msg->id << " center: (" << msg->x << ", " << msg->y << ")\n";
}

void * VxWindow::april_thread (void *arg) {
    lcm::LCM lcm;
    if (!lcm.good()) return NULL;

    VxWindow::Handler handlerObject;
    lcm.subscribe("APRIL_TAGS", &Handler::handleApril, &handlerObject);

    while (0 == lcm.handle());
    return NULL;
}

void VxWindow::render(void) {


    // Created world buffer
	vx_buffer_t* worldBuf = vx_world_get_buffer(world_, "map");
    // vx_buffer_add_back(worldBuf, vxo_grid());

    //Use the image source if it isn't null for the camera playback
    if (isrc != NULL) {
        //Retrieve the current frame of the image source
        //image_source_data_t *frmd = new image_source_data_t;
        image_source_data_t frmd;
        int res = isrc->get_frame (isrc, &frmd);
        if (res < 0)
            std::cout << "Get frame fail: " << res << "\n";

        else {

            //Convert the current frame to an image_u32
            char* pnm = "temp.pnm";
            image_u32_t *new_im = image_convert_u32 (&frmd);
            image_u32_write_pnm(new_im, pnm);

            // image_u8_t *image = image_u8_create_from_pnm(pnm);

            if (new_im != NULL) {

                // zarray_t *detections = apriltag_detector_detect(td, new_im);

                //Create a vx variable from the image_u32 object
                vx_object_t *vim = vxo_image_from_u32(new_im,
                                                      VXO_IMAGE_FLIPY,
                                                      VX_TEX_MIN_FILTER | VX_TEX_MAG_FILTER);

                // render the image centered at the origin and at a normalized scale of +/-1 unit in x-dir
                img_scale = 2./new_im->width;
                left_shift = new_im->width/2.;
                right_shift = new_im->height/2.;
                vx_buffer_add_back (worldBuf,
                                    vxo_chain (vxo_mat_scale3 (img_scale, img_scale, 1.0),
                                               vxo_mat_translate3 (-left_shift, -right_shift, 0.),
                                               vim));

                //Swap the old frame with the new frame
                vx_buffer_swap (worldBuf);
                image_u32_destroy (new_im);
                // image_u8_destroy (image);
            }
        }
        fflush (stdout);
        isrc->release_frame (isrc, &frmd);
    }
    ++frameCount_;
}
