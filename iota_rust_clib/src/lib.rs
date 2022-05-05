use iota_client::Client;
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;
use std::panic::catch_unwind;
use async_ffi::{FfiFuture, FutureExt};

#[no_mangle]
pub extern fn print_hello() 
{
    println!("hello world!");
}

fn my_string_safe(s: *const c_char) -> String {
    unsafe {
        CStr::from_ptr(s).to_string_lossy().into_owned()
    }
}

/*
fn long_running_task() -> u32 {
    thread::sleep(Duration::from_secs(5));
    5
}

async fn my_task() {
    let res = tokio::task::spawn_blocking(|| {
        long_running_task()
    }).await.unwrap();
    println!("The answer was: {}", res);
}
*/

pub async fn send_message(iota_url: String, index: String, message: String) -> Result<(), E>
{
    let iota = Client::builder()
        .with_node(&iota_url)?
        .finish()
        .await?;

    let message = iota
        .message()
        .with_index(&index)
        .with_data(message.as_bytes().to_vec())
        .finish()
        .await?;

    println!(
        "Message sent https://explorer.iota.org/testnet/message/{}\n",
        message.id().0
    );
    //copy_rust_to_c(message.id().0, message_id, 32);

    println!("Message sent {}", message.id().0);
    Ok(())
}

#[no_mangle]
pub extern fn iota_send_indiced_transaction(iota_url: *const c_char, index: *const c_char, message: *const c_char) {
        
    let future = async move {
        println!("*** future starting!!");
        send_message(my_string_safe(iota_url), my_string_safe(index), my_string_safe(message)).await;
    };
    
    let res = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()
            .unwrap()
            .block_on(future);
    
}
/*
#[no_mangle]
pub extern fn iota_send_indiced_transaction(iota_url: *const c_char, index: *const c_char, message: *const c_char) -> FfiFuture<u32>
{
    //let result = catch_unwind(|| {
        async move {
            let iota = Client::builder()
                .with_node(&my_string_safe(iota_url))?
                .finish()
                .await?;
                        
            let iotaMessage = iota_result.message()
                .with_index(&my_string_safe(index))
                .with_data(my_string_safe(message).as_bytes().to_vec())
                .finish()
                .await;
            
            println!("Message sent {}", iotaMessage.id().0);   
            return 0;
            /*unsafe {
                my_printer(c_to_print.as_ptr());
            }*/
            //return iotaMessage.id().as_ptr(); 
            
        }
        .into_ffi()
    //});
    /*match result {
        Ok(r) => 1,
        Err(_) => 2/*{
            //let c_string = CString::new("error").expect("CString::new failed");
            unsafe {
                //c_string.as_ptr()
            }
            
        },*/
    }*/
}
*/
#[no_mangle]
pub extern fn free_rust_string(rust_string: *mut c_char)
{
    let _ = CString::from_raw(rust_string);
}
