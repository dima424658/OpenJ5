use super::{Digest, PrepareError, Sha256, unavailable};

const TEMPLATE_SIZE: usize = 222;
const TEMPLATE_SHA256: [u8; 32] = [
    0x9a, 0xfe, 0x4f, 0x50, 0x24, 0xe2, 0x94, 0x79, 0xde, 0xdf, 0xbc, 0x28, 0xb5, 0xd6, 0x1d, 0xd2,
    0x7f, 0x64, 0x77, 0x32, 0x1b, 0xd4, 0xae, 0x62, 0x84, 0x21, 0x83, 0x8a, 0x63, 0x11, 0x15, 0xda,
];

const HANDLER_SIGNATURE: &[u8] = b"\x30\x48\xfd\xc2\x48\xd2\xae\xd2\xbe";

pub(super) fn apply(bytes: &mut [u8], template: &[u8]) -> Result<(), PrepareError> {
    if template.len() != TEMPLATE_SIZE || Sha256::digest(template)[..] != TEMPLATE_SHA256 {
        return Err(PrepareError::RelayTemplate);
    }

    // Hook, original handler and reserved template slot. These three BIN
    // layouts were checked against the original patch routine in isolation.
    let (hook, handler, slot, following) = match &bytes[0x6020..0x6028] {
        b"J5V05L19" => (0x5613, 0x29d7, 0x5ac0, [0x20, 0x63, 0x1a]),
        b"J5V05J16" => (0x3e55, 0x1baa, 0x55b0, [0xc2, 0x43, 0x85]),
        b"J5V05N35" => (0x3c57, 0x1b85, 0x5100, [0xc2, 0x43, 0x85]),
        _ => return Err(unavailable(bytes, "starter relay")),
    };
    let [high, low] = u16::to_be_bytes(handler as u16);
    if bytes[hook - 3..hook] != [0x44, 0x01, 0xf0]
        || bytes[hook..hook + 3] != [0x02, high, low]
        || bytes[hook + 3..hook + 6] != following
        || !bytes[handler..].starts_with(HANDLER_SIGNATURE)
        || bytes[slot..slot + 224].iter().any(|&byte| byte != 0)
    {
        return Err(unavailable(bytes, "starter relay"));
    }

    let mut code = [0; TEMPLATE_SIZE];
    code.copy_from_slice(template);
    // Relocations in the 2.18 starter routine, after decrypting its template.
    let table = slot + 105;
    for (offset, address) in [
        (1, 0xf999),
        (39, table),
        (53, table + 0x27),
        (79, table + 0x4e),
        (90, 0xf999),
        (97, handler),
    ] {
        code[offset..offset + 2].copy_from_slice(&(address as u16).to_be_bytes());
    }
    bytes[slot..slot + TEMPLATE_SIZE].copy_from_slice(&code);
    bytes[hook + 1..hook + 3].copy_from_slice(&(slot as u16).to_be_bytes());
    Ok(())
}
