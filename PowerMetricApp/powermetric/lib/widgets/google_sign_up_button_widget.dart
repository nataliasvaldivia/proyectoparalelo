import 'package:flutter/material.dart';
import 'package:font_awesome_flutter/font_awesome_flutter.dart';
import 'package:powermetric/providers/google_sign_in.dart';
import 'package:provider/provider.dart';
import 'package:google_fonts/google_fonts.dart';

class GoogleSignUpButtonWidget extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Container(
      padding: EdgeInsets.all(5),
      /*decoration: BoxDecoration(
          borderRadius: BorderRadius.circular(30),
          color: Colors.purple[700],
        ),*/
      child: ElevatedButton.icon(
          onPressed: () {
            final provider =
                Provider.of<GoogleSignInProvider>(context, listen: false);
            provider.login();
          },
          icon: FaIcon(FontAwesomeIcons.google, color: Colors.white),
          label: Text(
            'Sign in with Google',
            style: GoogleFonts.poppins(fontSize: 20, color: Colors.white),
          ),
          style: ElevatedButton.styleFrom(
            primary: Colors.red,
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(30),
            ),
            padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15),
          )),
    );
  }
}
