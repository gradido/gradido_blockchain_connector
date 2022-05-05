use iota_client::{Client, Error};
use std::ffi::CStr;
use std::ffi::CString;
use std::os::raw::c_char;
use serde::{Deserialize, Serialize};
//use serde_json::Result;

// https://github.com/serde-rs/json
#[derive(Serialize, Deserialize)]
struct JsonReturn {
    state: String,
    msg: Option<String>,
    message_id: Option<String>
}

#[no_mangle]
pub extern fn print_hello() 
{
    println!("Hello World");
}

fn my_string_safe(s: *const c_char) -> String {
    unsafe {
        CStr::from_ptr(s).to_string_lossy().into_owned()
    }
}


pub async fn send_message(iota_url: String, index: String, message: String) -> Result<String, Error>
{
    let iota = Client::builder()
        .with_node(&iota_url)?
        .with_local_pow(true)
        .finish()
        .await?;
    let info = iota.get_info().await?;
    //println!("Nodeinfo: {:?}", info);
/*
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
*/
  //  println!("Message sent {}", message.id().0);
    Ok(format!("Nodeinfo: {:?}", info))
}

#[no_mangle]
pub extern fn iota_send_indiced_transaction(iota_url: *const c_char, index: *const c_char, message: *const c_char) -> *mut c_char 
{        
    let future = async move {
        return send_message(my_string_safe(iota_url), my_string_safe(index), my_string_safe(message)).await;
    };
    
    let res = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()
            .unwrap()
            .block_on(future);    
    let json_result: JsonReturn; 
    match res {
        Ok(s) => {
            json_result= JsonReturn {
                state: "success".to_string(),
                msg: None,
                message_id: Some(s)
            }
        }
        Err(error) => {
            json_result= JsonReturn {
                state: "error".to_string(),
                msg: Some(error.to_string()),
                message_id: None
            }
        }
    }
   // Serialize it to a JSON string.
   let j = serde_json::to_string(&json_result);
   let c_string: CString;
   match j {
       Ok(j) => {
            c_string = CString::new(j).expect("CString::new failed");
       }
       Err(error) => {
            c_string = CString::new(format!("error with encoding result json: {}", error)).expect("CString::new failed");
       }
   }

   return c_string.into_raw();
}

#[no_mangle]
pub extern fn free_rust_string(rust_string: *mut c_char)
{
    unsafe {
        let _ = CString::from_raw(rust_string);
    }
}
